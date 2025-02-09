#include "NetworkUtils.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <iostream>

std::string clientKey(const sockaddr_in &c) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s:%d", inet_ntoa(c.sin_addr), ntohs(c.sin_port));
    return std::string(buf);
}

void sendAck(int sock, const sockaddr_in &addr, uint32_t seq) {
    AckPayload ack;
    ack.ackSequence = seq;

    MessageHeader h;
    h.type = static_cast<uint8_t>(MessageType::ACK);
    h.sequence = 0;
    h.timestamp = getCurrentTimeMS();
    h.flags = 0;

    char packet[sizeof(MessageHeader) + sizeof(AckPayload)];
    std::memcpy(packet, &h, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &ack, sizeof(AckPayload));

    sendto(sock, packet, sizeof(packet), 0,
           reinterpret_cast<struct sockaddr*>(const_cast<sockaddr_in*>(&addr)), sizeof(addr));
}

void broadcastPacket(int sock, const char* data, size_t len, const std::vector<sockaddr_in> &clients) {
    for (const auto &cl : clients) {
        int sent = sendto(sock, data, static_cast<int>(len), 0,
                          reinterpret_cast<struct sockaddr*>(const_cast<sockaddr_in*>(&cl)), sizeof(cl));
        if (sent < 0) {
            std::cerr << "[Server] broadcastPacket failed.\n";
        }
    }
}

void broadcastLobbyStatus(int sock, const std::vector<sockaddr_in> &clients, uint8_t total, uint8_t ready) {
    LobbyStatusPayload ls;
    ls.totalClients = total;
    ls.readyClients = ready;

    MessageHeader mh;
    mh.type = static_cast<uint8_t>(MessageType::LOBBY_STATUS);
    mh.sequence = 0;
    mh.timestamp = getCurrentTimeMS();
    mh.flags = 0;

    char packet[sizeof(MessageHeader) + sizeof(LobbyStatusPayload)];
    std::memcpy(packet, &mh, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &ls, sizeof(LobbyStatusPayload));

    broadcastPacket(sock, packet, sizeof(packet), clients);
}

void broadcastGameState(int sock, const std::vector<sockaddr_in> &clients, const GameStatePayload &gsPayload) {
    MessageHeader mh;
    mh.type = static_cast<uint8_t>(MessageType::GAME_STATE);
    mh.sequence = 0;
    mh.timestamp = getCurrentTimeMS();
    mh.flags = 1;

    char packet[sizeof(MessageHeader) + sizeof(GameStatePayload)];
    std::memcpy(packet, &mh, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &gsPayload, sizeof(GameStatePayload));

    broadcastPacket(sock, packet, sizeof(packet), clients);
}
