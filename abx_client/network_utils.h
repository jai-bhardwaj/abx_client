#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <string>
#include <vector>
#include <map>

int connectToServer(const std::string &ip, int port);
void sendRequest(int sockfd, uint8_t callType, uint8_t resendSeq = 0);
std::vector<std::map<std::string, std::string>> receivePackets(int sockfd);
std::vector<int> getMissingSequences(const std::vector<int> &receivedSequences);

#endif
