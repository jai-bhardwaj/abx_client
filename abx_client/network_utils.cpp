#include "network_utils.h"
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <algorithm>

// Connect to the server
int connectToServer(const std::string &ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        throw std::runtime_error("Connection failed");
    }
    return sockfd;
}

// Send a request to the server
void sendRequest(int sockfd, uint8_t callType, uint8_t resendSeq)
{
    uint8_t payload[2] = {callType, resendSeq};
    if (send(sockfd, payload, sizeof(payload), 0) < 0)
    {
        perror("Error sending request");
        close(sockfd);
        throw std::runtime_error("Failed to send request");
    }
}

// Receive packets from the server
std::vector<std::map<std::string, std::string>> receivePackets(int sockfd)
{
    std::vector<std::map<std::string, std::string>> packets;
    uint8_t buffer[17]; // Each packet is 17 bytes

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Error setting socket options");
        close(sockfd);
        throw std::runtime_error("Failed to set socket options");
    }

    while (true)
    {
        ssize_t bytesRead = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytesRead == 0)
            break; // Server closed the connection
        if (bytesRead < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                std::cerr << "Receive timeout" << std::endl;
                break;
            }
            perror("Error receiving data");
            close(sockfd);
            throw std::runtime_error("Failed to receive data");
        }

        if (bytesRead == sizeof(buffer)) // Ensure full packet is received
        {
            std::map<std::string, std::string> packet;
            packet["symbol"] = std::string(buffer, buffer + 4);                    // Symbol (4 bytes)
            packet["buySell"] = std::string(1, buffer[4]);                         // Buy/Sell (1 byte)
            packet["quantity"] = std::to_string(ntohl(*(int32_t *)(buffer + 5)));  // Quantity (4 bytes)
            packet["price"] = std::to_string(ntohl(*(int32_t *)(buffer + 9)));     // Price (4 bytes)
            packet["sequence"] = std::to_string(ntohl(*(int32_t *)(buffer + 13))); // Sequence (4 bytes)
            packets.push_back(packet);
        }
    }
    return packets;
}

// Identify missing sequences
std::vector<int> getMissingSequences(const std::vector<int> &receivedSequences)
{
    std::vector<int> missingSequences;
    if (receivedSequences.empty())
    {
        return missingSequences;
    }

    int maxSeq = *std::max_element(receivedSequences.begin(), receivedSequences.end());
    std::set<int> sequenceSet(receivedSequences.begin(), receivedSequences.end());
    for (int i = 1; i <= maxSeq; ++i)
    {
        if (sequenceSet.find(i) == sequenceSet.end())
        {
            missingSequences.push_back(i);
        }
    }
    return missingSequences;
}