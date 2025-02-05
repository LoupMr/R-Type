#ifndef NETWORKSYSTEM_HPP
#define NETWORKSYSTEM_HPP

#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

#include "Protocol.hpp"
#include "ECS/System.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ComponentManager.hpp"
#include "Components/Components.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

// track important packets awaiting ACK
struct PendingMessage {
    std::vector<char> data;
    std::chrono::steady_clock::time_point timeSent;
};

class NetworkSystem : public System {
public:
    NetworkSystem(const std::string &serverIP, int serverPort, int clientPort);
    ~NetworkSystem();

    // ECS system update
    void update(float dt, EntityManager &em, ComponentManager &cm) override;

    bool sendRaw(const std::string &data);

    // Sends a message payload optional
    bool sendPacket(uint8_t type, const void* payload, size_t payloadSize, bool important=false);

    // Sends an ACK for an important message
    bool sendAck(uint32_t sequence);

    // Whether we got a "start" from the server
    bool isGameStarted() const;

    // For drawing
    float getLatency() const;
    uint32_t getPacketLoss() const;

    // Unique ID for this client
    int getLocalNetworkID() const;

    // Lobby info
    void getLobbyStatus(uint8_t &total, uint8_t &ready);

private:
    void processPendingMessages();

private:
    int sock;
    sockaddr_in serverAddr;
    int localNetworkID;

    bool m_gameStarted = false;
    mutable std::mutex gameStartedMutex;

    std::mutex socketMutex;

    uint32_t nextSequence = 1;
    std::unordered_map<uint32_t, PendingMessage> pendingMessages;
    std::mutex pendingMutex;

    // Ping
    uint32_t lastPingSequence = 0;
    std::chrono::steady_clock::time_point pingSentTime;
    float latencyMs = 0.0f;

    // increment each time we re-send an important packet
    uint32_t packetLossCount = 0;

    // Remote Entities
    std::mutex remotePlayersMutex;
    std::mutex remoteEnemiesMutex;
    std::mutex remoteBulletsMutex;
    std::unordered_map<int, Entity> remotePlayers;
    std::unordered_map<int, Entity> remoteEnemies;
    std::unordered_map<int, Entity> remoteBullets;

    // Lobby
    uint8_t lobbyTotal = 0;
    uint8_t lobbyReady = 0;
    std::mutex lobbyMutex;
};

#endif // NETWORKSYSTEM_HPP
