#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include "network_utils.h" // Custom header for networking utilities
#include "json.hpp"        // JSON library (nlohmann/json)
#include <thread>
#include <sstream>
#include <mutex>
#include <unistd.h>

const std::string server_ip = "127.0.0.1"; // Server IP
const int server_port = 3000;              // Server port

using json = nlohmann::json;

std::mutex packetsMutex;

void saveToJson(const std::vector<std::map<std::string, std::string>> &packets, const std::string &filename)
{
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < packets.size(); ++i)
    {
        oss << "  {\n";
        for (auto it = packets[i].begin(); it != packets[i].end(); ++it)
        {
            oss << "    \"" << it->first << "\": \"" << it->second << "\"";
            if (std::next(it) != packets[i].end())
                oss << ",";
            oss << "\n";
        }
        oss << "  }";
        if (i != packets.size() - 1)
            oss << ",";
        oss << "\n";
    }
    oss << "]";

    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << oss.str();
        outFile.close();
        std::cout << "Data saved to " << filename << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}

void retrievePacket(const std::string &server_ip, int server_port, int seq, std::vector<std::map<std::string, std::string>> &packets)
{
    try
    {
        std::cout << "Connecting to server for sequence " << seq << "..." << std::endl;
        int resendSockfd = connectToServer(server_ip, server_port); // New connection for each missing sequence
        std::cout << "Connected to server for sequence " << seq << "." << std::endl;
        std::cout << "Sending request for sequence " << seq << "..." << std::endl;
        sendRequest(resendSockfd, 2, seq);
        std::cout << "Request sent for sequence " << seq << "." << std::endl;
        std::cout << "Receiving packets for sequence " << seq << "..." << std::endl;
        auto missingPacket = receivePackets(resendSockfd);
        std::cout << "Packets received for sequence " << seq << "." << std::endl;
        {
            std::lock_guard<std::mutex> lock(packetsMutex);
            packets.insert(packets.end(), missingPacket.begin(), missingPacket.end());
        }
        std::cout << "Closing connection for sequence " << seq << "..." << std::endl;
        close(resendSockfd);
        std::cout << "Connection closed for sequence " << seq << "." << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error retrieving sequence " << seq << ": " << ex.what() << std::endl;
    }
}

void processPackets(const std::string &server_ip, int server_port, const std::vector<int> &receivedSequences, std::vector<std::map<std::string, std::string>> &packets)
{
    std::cout << "Starting to retrieve missing sequences..." << std::endl;
    auto missingSequences = getMissingSequences(receivedSequences);
    std::cout << "Missing sequences retrieved." << std::endl;

    std::vector<std::thread> threads;
    for (int seq : missingSequences)
    {
        threads.emplace_back(retrievePacket, server_ip, server_port, seq, std::ref(packets));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    std::cout << "Saving packets to JSON..." << std::endl;
    saveToJson(packets, "output.json");
    std::cout << "Packets saved to JSON." << std::endl;
}

int main()
{
    try
    {
        // Step 1: Connect to the server
        std::cout << "Connecting to server..." << std::endl;
        int sockfd = connectToServer(server_ip, server_port);
        std::cout << "Connected to server." << std::endl;

        // Step 2: Request all packets (Call Type 1)
        std::cout << "Sending request for all packets..." << std::endl;
        sendRequest(sockfd, 1, 0);
        std::cout << "Request sent for all packets." << std::endl;

        // Step 3: Receive packets
        std::cout << "Receiving packets..." << std::endl;
        auto packets = receivePackets(sockfd);
        std::cout << "Packets received." << std::endl;
        close(sockfd); // Close the socket after receiving all packets
        std::cout << "Socket closed." << std::endl;

        // Step 4: Identify missing sequences
        std::vector<int> receivedSequences;
        for (const auto &packet : packets)
        {
            receivedSequences.push_back(std::stoi(packet.at("sequence")));
        }

        // Process packets to handle missing sequences
        processPackets(server_ip, server_port, receivedSequences, packets);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}