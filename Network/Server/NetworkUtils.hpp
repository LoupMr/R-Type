#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <netinet/in.h>
#include <vector>
#include <cstdint>
#include <string>
#include "../Protocol/Protocol.hpp"

std::string clientKey(const sockaddr_in &c);
void sendAck(int sock, const sockaddr_in &addr, uint32_t seq);
void broadcastPacket(int sock, const char* data, size_t len, const std::vector<sockaddr_in> &clients);
void broadcastLobbyStatus(int sock, const std::vector<sockaddr_in> &clients, uint8_t total, uint8_t ready);
void broadcastGameState(int sock, const std::vector<sockaddr_in> &clients, const GameStatePayload &gsPayload);

#endif // NETWORK_UTILS_HPP
