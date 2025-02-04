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

#include "NetworkSystem.hpp"
#include "Protocol.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ComponentManager.hpp"
#include "Components/Components.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <unordered_set>

NetworkSystem::NetworkSystem(const std::string &serverIP, int serverPort, int clientPort)
    : localNetworkID(clientPort)
{
#ifdef _WIN32
    WSADATA wsaData;
    int wsRes = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (wsRes != 0) {
        std::cerr << "[NetworkSystem] WSAStartup failed: " << wsRes << "\n";
    }
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[NetworkSystem] Error creating socket.\n";
    }

    sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port   = htons(clientPort);
    clientAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        std::cerr << "[NetworkSystem] Bind failed.\n";
    }

    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(serverPort);
#ifdef _WIN32
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);
#else
    inet_aton(serverIP.c_str(), &serverAddr.sin_addr);
#endif

    std::cout << "[NetworkSystem] Initialized. LocalID=" << localNetworkID
              << ", server=" << serverIP << ":" << serverPort << "\n";
}

NetworkSystem::~NetworkSystem() {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

bool NetworkSystem::sendRaw(const std::string &data) {
    std::lock_guard<std::mutex> lock(socketMutex);
    int sent = sendto(sock, data.c_str(), (int)data.size(), 0,
                      reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (sent < 0) {
        std::cerr << "[NetworkSystem] sendRaw failed.\n";
        return false;
    }
    return true;
}

bool NetworkSystem::sendPacket(uint8_t type, const void* payload, size_t payloadSize, bool important) {
    MessageHeader header;
    header.type      = type;
    header.sequence  = nextSequence++;
    header.timestamp = getCurrentTimeMS();
    header.flags     = (important ? 1 : 0);

    size_t totalSize = sizeof(MessageHeader) + payloadSize;
    std::vector<char> packet(totalSize);
    std::memcpy(packet.data(), &header, sizeof(MessageHeader));
    if (payload && payloadSize > 0) {
        std::memcpy(packet.data() + sizeof(MessageHeader), payload, payloadSize);
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        int sentBytes = sendto(sock, packet.data(), (int)totalSize, 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            std::cerr << "[NetworkSystem] sendto failed.\n";
            return false;
        }
    }

    if (important) {
        PendingMessage pm;
        pm.data = packet;
        pm.timeSent = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(pendingMutex);
        pendingMessages[header.sequence] = pm;
    }

    return true;
}

bool NetworkSystem::sendAck(uint32_t sequence) {
    AckPayload ack;
    ack.ackSequence = sequence;
    return sendPacket(MSG_ACK, &ack, sizeof(ack), false);
}

void NetworkSystem::processPendingMessages() {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(pendingMutex);

    for (auto &pair : pendingMessages) {
        auto &pending = pair.second;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - pending.timeSent).count();
        // If 100ms have passed without an ACK, re-send
        if (elapsed > 100) {
            // Count as "packet loss" for demonstration
            packetLossCount++;

            std::lock_guard<std::mutex> sockLock(socketMutex);
            int sentBytes = sendto(sock, pending.data.data(),
                                   (int)pending.data.size(), 0,
                                   reinterpret_cast<struct sockaddr*>(&serverAddr),
                                   sizeof(serverAddr));
            if (sentBytes >= 0) {
                pending.timeSent = now; // reset timer
            } else {
                std::cerr << "[NetworkSystem] Resend failed.\n";
            }
        }
    }
}

void NetworkSystem::update(float dt, EntityManager &em, ComponentManager &cm) {
    // Non-blocking receive
    char buffer[2048];
    int bytesReceived = 0;
    {
        std::lock_guard<std::mutex> lock(socketMutex);
#ifdef _WIN32
        bytesReceived = recvfrom(sock, buffer, (int)sizeof(buffer), MSG_DONTWAIT, nullptr, nullptr);
#else
        bytesReceived = recvfrom(sock, buffer, sizeof(buffer), MSG_DONTWAIT, nullptr, nullptr);
#endif
    }

    if (bytesReceived >= (int)sizeof(MessageHeader)) {
        MessageHeader header;
        std::memcpy(&header, buffer, sizeof(MessageHeader));
        uint32_t seq = header.sequence;

        // If it's important, send ack
        if (header.flags & 1) {
            sendAck(seq);
        }

        switch (header.type) {
            case MSG_ACK: {
                if (bytesReceived >= (int)(sizeof(MessageHeader) + sizeof(AckPayload))) {
                    AckPayload ack;
                    std::memcpy(&ack, buffer + sizeof(MessageHeader), sizeof(AckPayload));
                    // Remove from pending
                    std::lock_guard<std::mutex> lock(pendingMutex);
                    pendingMessages.erase(ack.ackSequence);
                }
                break;
            }
            case MSG_START: {
                {
                    std::lock_guard<std::mutex> lock(gameStartedMutex);
                    m_gameStarted = true;
                }
                std::cout << "[NetworkSystem] Received MSG_START => gameStarted=true.\n";
                break;
            }
            case MSG_PONG: {
                // Measure round-trip
                if (bytesReceived >= (int)(sizeof(MessageHeader) + sizeof(PingPayload))) {
                    PingPayload pong;
                    std::memcpy(&pong, buffer + sizeof(MessageHeader), sizeof(PingPayload));
                    if (pong.pingSequence == lastPingSequence) {
                        auto now = std::chrono::steady_clock::now();
                        float ms = std::chrono::duration<float, std::milli>(now - pingSentTime).count();
                        latencyMs = ms;
                    }
                }
                break;
            }
            case MSG_GAME_STATE: {
                // interpret the full game state
                if (bytesReceived >= (int)(sizeof(MessageHeader) + sizeof(GameStatePayload))) {
                    GameStatePayload gs;
                    std::memcpy(&gs, buffer + sizeof(MessageHeader), sizeof(GameStatePayload));

                    // 1) Enemies
                    {
                        std::lock_guard<std::mutex> lock(remoteEnemiesMutex);
                        std::unordered_set<int> updated;
                        for (int i=0; i<gs.numEnemies; i++) {
                            int eID = gs.enemies[i].enemyID;
                            updated.insert(eID);

                            // If dead?
                            if (gs.enemies[i].health <= 0) {
                                // remove if exists
                                if (remoteEnemies.count(eID)) {
                                    em.destroyEntity(remoteEnemies[eID]);
                                    remoteEnemies.erase(eID);
                                }
                                continue;
                            }

                            // create or update
                            if (!remoteEnemies.count(eID)) {
                                Entity eEnt = em.createEntity();
                                cm.addComponent(eEnt, Position{gs.enemies[i].x, gs.enemies[i].y});
                                Texture2D tex = cm.getGlobalTexture("enemy");
                                cm.addComponent(eEnt, Sprite{tex, tex.width, tex.height});
                                remoteEnemies[eID] = eEnt;
                            } else {
                                Entity eEnt = remoteEnemies[eID];
                                if (auto *pos = cm.getComponent<Position>(eEnt)) {
                                    pos->x = gs.enemies[i].x;
                                    pos->y = gs.enemies[i].y;
                                }
                            }
                        }
                        // remove stale
                        for (auto it = remoteEnemies.begin(); it != remoteEnemies.end();) {
                            if (updated.find(it->first) == updated.end()) {
                                em.destroyEntity(it->second);
                                it = remoteEnemies.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }

                    // 2) Players
                    {
                        std::lock_guard<std::mutex> lock(remotePlayersMutex);
                        std::unordered_set<int> updated;
                        for (int i=0; i<gs.numPlayers; i++) {
                            int pid = gs.players[i].playerID;
                            updated.insert(pid);

                            if (gs.players[i].health <= 0) {
                                if (remotePlayers.count(pid)) {
                                    em.destroyEntity(remotePlayers[pid]);
                                    remotePlayers.erase(pid);
                                }
                                continue;
                            }

                            if (!remotePlayers.count(pid)) {
                                // create
                                Entity pEnt = em.createEntity();
                                cm.addComponent(pEnt, Position{gs.players[i].x, gs.players[i].y});
                                Texture2D rpTex = cm.getGlobalTexture("remotePlayer");
                                cm.addComponent(pEnt, Sprite{rpTex, rpTex.width, rpTex.height});
                                remotePlayers[pid] = pEnt;
                            } else {
                                // update
                                Entity pEnt = remotePlayers[pid];
                                if (auto *pos = cm.getComponent<Position>(pEnt)) {
                                    pos->x = gs.players[i].x;
                                    pos->y = gs.players[i].y;
                                }
                            }
                        }
                        // remove stale
                        for (auto it = remotePlayers.begin(); it != remotePlayers.end();) {
                            if (updated.find(it->first) == updated.end()) {
                                em.destroyEntity(it->second);
                                it = remotePlayers.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }

                    // 3) Bullets
                    {
                        std::lock_guard<std::mutex> lock(remoteBulletsMutex);
                        std::unordered_set<int> updated;
                        for (int i=0; i<gs.numBullets; i++) {
                            auto &b = gs.bullets[i];
                            updated.insert(b.bulletID);

                            if (!remoteBullets.count(b.bulletID)) {
                                Entity bEnt = em.createEntity();
                                cm.addComponent(bEnt, Position{b.x, b.y});
                                Texture2D bulletTex = cm.getGlobalTexture("bullet");
                                cm.addComponent(bEnt, Sprite{bulletTex, bulletTex.width, bulletTex.height});
                                remoteBullets[b.bulletID] = bEnt;
                            } else {
                                Entity bEnt = remoteBullets[b.bulletID];
                                if (auto *pos = cm.getComponent<Position>(bEnt)) {
                                    pos->x = b.x;
                                    pos->y = b.y;
                                }
                            }
                        }
                        // remove stale
                        for (auto it = remoteBullets.begin(); it != remoteBullets.end();) {
                            if (updated.find(it->first) == updated.end()) {
                                em.destroyEntity(it->second);
                                it = remoteBullets.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                }
                break;
            }
            case MSG_LOBBY_STATUS: {
                if (bytesReceived >= (int)(sizeof(MessageHeader) + sizeof(LobbyStatusPayload))) {
                    LobbyStatusPayload ls;
                    std::memcpy(&ls, buffer + sizeof(MessageHeader), sizeof(LobbyStatusPayload));
                    std::lock_guard<std::mutex> lock(lobbyMutex);
                    lobbyTotal = ls.totalClients;
                    lobbyReady = ls.readyClients;
                }
                break;
            }
            default:
                break;
        }
    }

    // Resend important messages if not acked
    processPendingMessages();

    // Periodic ping (once per second)
    static float pingTimer = 0.0f;
    pingTimer += dt;
    if (pingTimer >= 1.0f) {
        pingTimer = 0.0f;
        lastPingSequence++;
        PingPayload pp;
        pp.pingSequence = lastPingSequence;
        pingSentTime = std::chrono::steady_clock::now();
        sendPacket(MSG_PING, &pp, sizeof(pp), false);
    }
}

bool NetworkSystem::isGameStarted() const {
    std::lock_guard<std::mutex> lock(gameStartedMutex);
    return m_gameStarted;
}

float NetworkSystem::getLatency() const {
    return latencyMs;
}

uint32_t NetworkSystem::getPacketLoss() const {
    return packetLossCount;
}

int NetworkSystem::getLocalNetworkID() const {
    return localNetworkID;
}

void NetworkSystem::getLobbyStatus(uint8_t &total, uint8_t &ready) {
    std::lock_guard<std::mutex> lock(lobbyMutex);
    total = lobbyTotal;
    ready = lobbyReady;
}
