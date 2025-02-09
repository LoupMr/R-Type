#include "NetworkSystem.hpp"
#include <iostream>
#include <cstring>
#include <unordered_set>

NetworkSystem::NetworkSystem(const std::string &serverIP, int serverPort, int clientPort)
    : localNetworkID(clientPort)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[NetworkSystem] Error creating socket.\n";
    }

    sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port   = htons(clientPort);
    clientAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr)) < 0) {
        std::cerr << "[NetworkSystem] Bind failed.\n";
    }

    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(serverPort);
    inet_aton(serverIP.c_str(), &serverAddr.sin_addr);

    std::cout << "[NetworkSystem] Initialized. LocalID=" << localNetworkID
              << ", server=" << serverIP << ":" << serverPort << "\n";
}

NetworkSystem::~NetworkSystem() {
    close(sock);
}

bool NetworkSystem::sendRaw(const std::string &data) {
    std::lock_guard<std::mutex> lock(socketMutex);
    int sent = sendto(sock, data.c_str(), static_cast<int>(data.size()), 0,
                      reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (sent < 0) {
        std::cerr << "[NetworkSystem] sendRaw failed.\n";
        return false;
    }
    return true;
}

bool NetworkSystem::sendPacket(uint8_t type, const void* payload, size_t payloadSize, bool important) {
    MessageHeader header;
    header.type = type;
    header.sequence = nextSequence++;
    header.timestamp = getCurrentTimeMS();
    header.flags = important ? 1 : 0;

    size_t totalSize = sizeof(MessageHeader) + payloadSize;
    std::vector<char> packet(totalSize);
    std::memcpy(packet.data(), &header, sizeof(MessageHeader));
    if (payload && payloadSize > 0) {
        std::memcpy(packet.data() + sizeof(MessageHeader), payload, payloadSize);
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex);
        int sentBytes = sendto(sock, packet.data(), static_cast<int>(totalSize), 0,
                               reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
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
    return sendPacket(static_cast<uint8_t>(MessageType::ACK), &ack, sizeof(ack), false);
}

void NetworkSystem::processPendingMessages() {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(pendingMutex);
    for (auto &pair : pendingMessages) {
        auto &pending = pair.second;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pending.timeSent).count();
        if (elapsed > 100) {
            packetLossCount++;
            {
                std::lock_guard<std::mutex> sockLock(socketMutex);
                int sentBytes = sendto(sock, pending.data.data(), static_cast<int>(pending.data.size()), 0,
                                       reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
                if (sentBytes >= 0) {
                    pending.timeSent = now;
                } else {
                    std::cerr << "[NetworkSystem] Resend failed.\n";
                }
            }
        }
    }
}

void NetworkSystem::update(float dt, Engine::EntityManager &em, Engine::ComponentManager &cm) {
    char buffer[2048];
    int bytesReceived = 0;
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        bytesReceived = recvfrom(sock, buffer, sizeof(buffer), MSG_DONTWAIT, nullptr, nullptr);
    }

    if (bytesReceived >= static_cast<int>(sizeof(MessageHeader))) {
        MessageHeader header;
        std::memcpy(&header, buffer, sizeof(MessageHeader));
        uint32_t seq = header.sequence;

        if (header.flags & 1)
            sendAck(seq);

        switch (header.type) {
            case static_cast<uint8_t>(MessageType::ACK): {
                if (bytesReceived >= static_cast<int>(sizeof(MessageHeader) + sizeof(AckPayload))) {
                    AckPayload ack;
                    std::memcpy(&ack, buffer + sizeof(MessageHeader), sizeof(AckPayload));
                    std::lock_guard<std::mutex> lock(pendingMutex);
                    pendingMessages.erase(ack.ackSequence);
                }
                break;
            }
            case static_cast<uint8_t>(MessageType::START): {
                {
                    std::lock_guard<std::mutex> lock(gameStartedMutex);
                    m_gameStarted = true;
                }
                std::cout << "[NetworkSystem] Received MSG_START: gameStarted set to true.\n";
                break;
            }
            case static_cast<uint8_t>(MessageType::PONG): {
                if (bytesReceived >= static_cast<int>(sizeof(MessageHeader) + sizeof(PingPayload))) {
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
            case static_cast<uint8_t>(MessageType::GAME_STATE): {
                if (bytesReceived >= static_cast<int>(sizeof(MessageHeader) + sizeof(GameStatePayload))) {
                    GameStatePayload gs;
                    std::memcpy(&gs, buffer + sizeof(MessageHeader), sizeof(GameStatePayload));

                    {
                        std::lock_guard<std::mutex> lock(remoteEnemiesMutex);
                        std::unordered_set<int> updated;
                        for (int i = 0; i < gs.numEnemies; i++) {
                            int eID = gs.enemies[i].enemyID;
                            updated.insert(eID);
                            if (gs.enemies[i].health <= 0) {
                                if (remoteEnemies.count(eID)) {
                                    em.destroyEntity(remoteEnemies[eID]);
                                    remoteEnemies.erase(eID);
                                }
                                continue;
                            }
                            if (!remoteEnemies.count(eID)) {
                                Engine::Entity eEnt = em.createEntity();
                                cm.addComponent(eEnt, Position{gs.enemies[i].x, gs.enemies[i].y});
                                auto tex = cm.getGlobalTexture("enemy");
                                cm.addComponent(eEnt, Sprite{tex, tex.width, tex.height});
                                remoteEnemies[eID] = eEnt;
                            } else {
                                Engine::Entity eEnt = remoteEnemies[eID];
                                if (auto *pos = cm.getComponent<Position>(eEnt)) {
                                    pos->x = gs.enemies[i].x;
                                    pos->y = gs.enemies[i].y;
                                }
                            }
                        }
                        for (auto it = remoteEnemies.begin(); it != remoteEnemies.end();) {
                            if (updated.find(it->first) == updated.end()) {
                                em.destroyEntity(it->second);
                                it = remoteEnemies.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(remotePlayersMutex);
                        std::unordered_set<int> updated;
                        for (int i = 0; i < gs.numPlayers; i++) {
                            int pid = gs.players[i].playerID;
                            if (pid == getLocalNetworkID()) {
                                if (auto *pos = cm.getComponent<Position>(0)) {
                                    pos->x = gs.players[i].x;
                                    pos->y = gs.players[i].y;
                                }
                                if (auto *hp = cm.getComponent<Health>(0)) {
                                    hp->current = gs.players[i].health;
                                }
                                if (remotePlayers.count(pid)) {
                                    em.destroyEntity(remotePlayers[pid]);
                                    remotePlayers.erase(pid);
                                }
                                continue;
                            }
                            updated.insert(pid);
                            if (gs.players[i].health <= 0) {
                                if (remotePlayers.count(pid)) {
                                    em.destroyEntity(remotePlayers[pid]);
                                    remotePlayers.erase(pid);
                                }
                                continue;
                            }
                            if (!remotePlayers.count(pid)) {
                                Engine::Entity pEnt = em.createEntity();
                                cm.addComponent(pEnt, Position{gs.players[i].x, gs.players[i].y});
                                auto rpTex = cm.getGlobalTexture("remotePlayer");
                                cm.addComponent(pEnt, Sprite{rpTex, rpTex.width, rpTex.height});
                                remotePlayers[pid] = pEnt;
                            } else {
                                Engine::Entity pEnt = remotePlayers[pid];
                                if (auto *pos = cm.getComponent<Position>(pEnt)) {
                                    pos->x = gs.players[i].x;
                                    pos->y = gs.players[i].y;
                                }
                            }
                        }
                        for (auto it = remotePlayers.begin(); it != remotePlayers.end();) {
                            if (updated.find(it->first) == updated.end()) {
                                em.destroyEntity(it->second);
                                it = remotePlayers.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(remoteBulletsMutex);
                        std::unordered_set<int> updated;
                        for (int i = 0; i < gs.numBullets; i++) {
                            auto &b = gs.bullets[i];
                            updated.insert(b.bulletID);
                            if (!remoteBullets.count(b.bulletID)) {
                                Engine::Entity bEnt = em.createEntity();
                                cm.addComponent(bEnt, Position{b.x, b.y});
                                auto bulletTex = cm.getGlobalTexture("bullet");
                                cm.addComponent(bEnt, Sprite{bulletTex, bulletTex.width, bulletTex.height});
                                remoteBullets[b.bulletID] = bEnt;
                            } else {
                                Engine::Entity bEnt = remoteBullets[b.bulletID];
                                if (auto *pos = cm.getComponent<Position>(bEnt)) {
                                    pos->x = b.x;
                                    pos->y = b.y;
                                }
                            }
                        }
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
            case static_cast<uint8_t>(MessageType::LOBBY_STATUS): {
                if (bytesReceived >= static_cast<int>(sizeof(MessageHeader) + sizeof(LobbyStatusPayload))) {
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

    processPendingMessages();

    static float pingTimer = 0.0f;
    pingTimer += dt;
    if (pingTimer >= 1.0f) {
        pingTimer = 0.0f;
        lastPingSequence++;
        PingPayload pp;
        pp.pingSequence = lastPingSequence;
        pingSentTime = std::chrono::steady_clock::now();
        sendPacket(static_cast<uint8_t>(MessageType::PING), &pp, sizeof(pp), false);
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
