#ifndef NETWORKSYSTEM_HPP
#define NETWORKSYSTEM_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

#include "../Protocol/Protocol.hpp"

#include "Engine/ECS/System.hpp"
#include "Engine/ECS/EntityManager.hpp"
#include "Engine/ECS/ComponentManager.hpp"
#include "Game/Components/Components.hpp"

struct PendingMessage {
    std::vector<char> data;
    std::chrono::steady_clock::time_point timeSent;
};

class NetworkSystem : public Engine::System {
public:
    NetworkSystem(const std::string &serverIP, int serverPort, int clientPort);
    ~NetworkSystem();

    void update(float dt, Engine::EntityManager &em, Engine::ComponentManager &cm) override;

    bool sendRaw(const std::string &data);
    bool sendPacket(uint8_t type, const void* payload, size_t payloadSize, bool important = false);
    bool sendAck(uint32_t sequence);

    bool isGameStarted() const;
    float getLatency() const;
    uint32_t getPacketLoss() const;
    int getLocalNetworkID() const;

    void getLobbyStatus(uint8_t &total, uint8_t &ready);

private:
    void processPendingMessages();
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

    uint32_t packetLossCount = 0;

    // Remote Entities
    std::mutex remotePlayersMutex;
    std::mutex remoteEnemiesMutex;
    std::mutex remoteBulletsMutex;
    std::unordered_map<int, Engine::Entity> remotePlayers;
    std::unordered_map<int, Engine::Entity> remoteEnemies;
    std::unordered_map<int, Engine::Entity> remoteBullets;

    // Lobby
    uint8_t lobbyTotal = 0;
    uint8_t lobbyReady = 0;
    std::mutex lobbyMutex;
};

#endif // NETWORKSYSTEM_HPP
