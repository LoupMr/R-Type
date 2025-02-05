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

#include "Systems/Protocol.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cmath>

std::mutex clientsMutex;
std::mutex stateMutex;

// To Do block client port if already use
// ----------------------------------------
// Player
// ----------------------------------------
struct PlayerStateInfo {
    float x;
    float y;
    int32_t health;
};

// ----------------------------------------
// Enemy
// ----------------------------------------
struct EnemyStateInfo {
    int32_t enemyID;
    float   x;
    float   y;
    float   vx;
    int32_t health;
    bool    active;
    float   shootTimer; // countdown to next shot
};

// ----------------------------------------
// Bullet
// ----------------------------------------
struct BulletInfo {
    int32_t bulletID;
    float   x;
    float   y;
    float   vx;
    float   vy;
    int32_t ownerID; // >=0 => player bullet, <0 => enemy bullet
    bool    active;
};

// ----------------------------------------
// Global data
// ----------------------------------------
std::unordered_map<int, PlayerStateInfo> playerStates;
std::unordered_map<int, PlayerInputPayload> playerInputs;

std::vector<EnemyStateInfo> enemies;
std::vector<BulletInfo> bullets;

static int32_t nextEnemyID  = 1;
static int32_t nextBulletID = 1000;

float waveTimer = 0.0f;
int   waveIndex = 0;

float spawnInterval  = 10.0f;  // spawn wave every 10s
int   spawnCount     = 3;      // enemies per wave
float enemySpeed     = -50.0f; // px/sec
float enemyShootTime = 2.0f;   // shoot every 2s
float bulletSpeed    = 200.f;  // bullet speed

// ----------------------------------------
// Utility
// ----------------------------------------
bool clientExists(const std::vector<sockaddr_in> &clients, const sockaddr_in &c) {
    for (auto &cc : clients) {
        if (cc.sin_addr.s_addr == c.sin_addr.s_addr &&
            cc.sin_port == c.sin_port) {
            return true;
        }
    }
    return false;
}

std::string clientKey(const sockaddr_in &c) {
    char buf[64];
    sprintf(buf, "%s:%d", inet_ntoa(c.sin_addr), ntohs(c.sin_port));
    return buf;
}

void sendAck(int sock, const sockaddr_in &addr, uint32_t seq) {
    AckPayload ack;
    ack.ackSequence = seq;

    MessageHeader h;
    h.type = MSG_ACK;
    h.sequence = 0;
    h.timestamp = getCurrentTimeMS();
    h.flags = 0;

    char packet[sizeof(MessageHeader) + sizeof(AckPayload)];
    std::memcpy(packet, &h, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &ack, sizeof(AckPayload));

    sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
}

void broadcastPacket(int sock, const char* data, size_t len, const std::vector<sockaddr_in> &clients) {
    for (auto &cl : clients) {
        int sent = sendto(sock, data, (int)len, 0, (struct sockaddr*)&cl, sizeof(cl));
        if (sent < 0) {
            std::cerr << "[Server] broadcastPacket failed.\n";
        }
    }
}

void broadcastSimple(int sock, uint8_t msgType, bool important, const std::vector<sockaddr_in> &clients) {
    MessageHeader h;
    h.type = msgType;
    h.sequence = 0;
    h.timestamp = getCurrentTimeMS();
    h.flags = important ? 1 : 0;

    for (auto &cl : clients) {
        sendto(sock, (const char*)&h, sizeof(h), 0, (struct sockaddr*)&cl, sizeof(cl));
    }
}

void broadcastLobbyStatus(int sock, const std::vector<sockaddr_in> &clients,
                          uint8_t total, uint8_t ready)
{
    LobbyStatusPayload ls;
    ls.totalClients = total;
    ls.readyClients = ready;

    MessageHeader mh;
    mh.type = MSG_LOBBY_STATUS;
    mh.sequence = 0;
    mh.timestamp = getCurrentTimeMS();
    mh.flags = 0;

    char packet[sizeof(MessageHeader) + sizeof(LobbyStatusPayload)];
    std::memcpy(packet, &mh, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &ls, sizeof(LobbyStatusPayload));

    broadcastPacket(sock, packet, sizeof(packet), clients);
}

bool checkCollision(float x1, float y1, float x2, float y2, float radius=20.f) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return (dx*dx + dy*dy) < (radius * radius);
}

// ----------------------------------------
// Spawn wave of enemies on the right side
// ----------------------------------------
void spawnWave() {
    for (int i=0; i<spawnCount; i++) {
        EnemyStateInfo e;
        e.enemyID    = nextEnemyID++;
        e.x          = 850.f;        // off-screen to the right
        e.y          = 100.f + i*80; // vertical spacing
        e.vx         = enemySpeed;
        e.health     = 3;
        e.active     = true;
        e.shootTimer = enemyShootTime;
        enemies.push_back(e);
    }
    std::cout << "[Server] Spawn wave " << waveIndex << " with " << spawnCount << " enemies.\n";
}

void broadcastGameState(int sock, const std::vector<sockaddr_in> &clients) {
    GameStatePayload gs;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        // 1) Enemies
        int eCount = 0;
        for (auto &e : enemies) {
            if (!e.active) continue;
            if (eCount >= 32) break;
            gs.enemies[eCount].enemyID = e.enemyID;
            gs.enemies[eCount].x       = e.x;
            gs.enemies[eCount].y       = e.y;
            gs.enemies[eCount].health  = e.health;
            eCount++;
        }
        gs.numEnemies = (uint8_t)eCount;

        // 2) Players
        int pCount = 0;
        for (auto &p : playerStates) {
            if (pCount >= 4) break;
            gs.players[pCount].playerID = p.first;
            gs.players[pCount].x        = p.second.x;
            gs.players[pCount].y        = p.second.y;
            gs.players[pCount].health   = p.second.health;
            pCount++;
        }
        gs.numPlayers = (uint8_t)pCount;

        // 3) Bullets
        int bCount = 0;
        for (auto &b : bullets) {
            if (!b.active) continue;
            if (bCount >= 32) break;
            gs.bullets[bCount].bulletID = b.bulletID;
            gs.bullets[bCount].x        = b.x;
            gs.bullets[bCount].y        = b.y;
            gs.bullets[bCount].vx       = b.vx;
            gs.bullets[bCount].vy       = b.vy;
            gs.bullets[bCount].ownerID  = b.ownerID;
            bCount++;
        }
        gs.numBullets = (uint8_t)bCount;
    }

    MessageHeader mh;
    mh.type      = MSG_GAME_STATE;
    mh.sequence  = 0;
    mh.timestamp = getCurrentTimeMS();
    mh.flags     = 1; // important

    char packet[sizeof(MessageHeader) + sizeof(GameStatePayload)];
    std::memcpy(packet, &mh, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &gs, sizeof(GameStatePayload));

    broadcastPacket(sock, packet, sizeof(packet), clients);
}

// ----------------------------------------
// Main game simulation
// ----------------------------------------
void simulationLoop(int sock, std::vector<sockaddr_in> &clients, bool &gameStarted) {
    while (true) {
        if (gameStarted) {
            float dt = 0.09f; // stepping every xxms

            {
                std::lock_guard<std::mutex> lock(stateMutex);

                // Wave spawn
                waveTimer += dt;
                if (waveTimer >= spawnInterval) {
                    waveTimer = 0.0f;
                    waveIndex++;
                    spawnWave();
                }

                // Update enemies
                for (auto &e : enemies) {
                    if (!e.active) continue;
                    e.x += e.vx * dt;
                    // off-screen left => detroyed
                    if (e.x < -100) {
                        e.active = false;
                    }
                    // shooting
                    e.shootTimer -= dt;
                    if (e.shootTimer <= 0.f) {
                        // spawn bullet
                        BulletInfo b;
                        b.bulletID = nextBulletID++;
                        b.x = e.x;
                        b.y = e.y;
                        b.vx = -bulletSpeed;
                        b.vy = 0.f;
                        b.ownerID = -e.enemyID; // negative => enemy bullet
                        b.active = true;
                        bullets.push_back(b);
                        e.shootTimer = enemyShootTime;
                    }
                }

                // Update players from their inputs
                for (auto &kv : playerInputs) {
                    int pid = kv.first;
                    auto &inp = kv.second;
                    auto &ps  = playerStates[pid];

                    float speed = 150.f;
                    if (inp.up)    ps.y -= speed * dt;
                    if (inp.down)  ps.y += speed * dt;
                    if (inp.left)  ps.x -= speed * dt;
                    if (inp.right) ps.x += speed * dt;

                    if (inp.shoot) {
                        // spawn bullet
                        BulletInfo b;
                        b.bulletID = nextBulletID++;
                        b.x = ps.x;
                        b.y = ps.y;
                        b.vx = bulletSpeed;
                        b.vy = 0.f;
                        b.ownerID = pid; // pos player bullet
                        b.active = true;
                        bullets.push_back(b);
                        inp.shoot = false;
                    }
                }

                // Update bullets
                for (auto &b : bullets) {
                    if (!b.active) continue;
                    b.x += b.vx * dt;
                    b.y += b.vy * dt;

                    // off-screen
                    if (b.x < -50 || b.x > 850 || b.y < 0 || b.y > 600) {
                        b.active = false;
                        continue;
                    }

                    // Collisions
                    if (b.ownerID >= 0) {
                        // bullet from player => check enemies
                        for (auto &e : enemies) {
                            if (!e.active || e.health <= 0) continue;
                            if (checkCollision(b.x, b.y, e.x, e.y, 20.f)) {
                                e.health--;
                                if (e.health <= 0) {
                                    e.active = false;
                                    std::cout << "[Server] Enemy " << e.enemyID << " killed.\n";
                                }
                                b.active = false;
                                break;
                            }
                        }
                    } else {
                        // bullet from enemy => check players
                        for (auto &p : playerStates) {
                            if (p.second.health <= 0) continue;
                            if (checkCollision(b.x, b.y, p.second.x, p.second.y, 20.f)) {
                                p.second.health--;
                                std::cout << "[Server] Player " << p.first
                                          << " took damage => HP=" << p.second.health << "\n";
                                b.active = false;
                                break;
                            }
                        }
                    }
                }
            }
            // broadcast
            broadcastGameState(sock, clients);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ----------------------------------------
// Thread receives packets
// ----------------------------------------
void handleClient(int sock, std::vector<sockaddr_in> &clients,
                  std::unordered_set<std::string> &readySet,
                  bool &gameStarted)
{
    char buffer[1024];
    while (true) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif
        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0,
                             (struct sockaddr*)&clientAddr, &addrLen);
        if (bytes < 0) {
            std::cerr << "[Server] recvfrom failed.\n";
            continue;
        }
        if (bytes < (int)sizeof(MessageHeader)) {
            continue;
        }

        MessageHeader inHeader;
        std::memcpy(&inHeader, buffer, sizeof(MessageHeader));
        uint32_t seq = inHeader.sequence;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (!clientExists(clients, clientAddr)) {
                clients.push_back(clientAddr);
                std::cout << "[Server] New client: " << clientKey(clientAddr) << "\n";
            }
        }

        if (inHeader.flags & 1) {
            sendAck(sock, clientAddr, seq);
        }

        switch (inHeader.type) {
            case MSG_READY: {
                std::lock_guard<std::mutex> lock(clientsMutex);
                readySet.insert(clientKey(clientAddr));
                size_t total = clients.size();
                size_t ready = readySet.size();
                std::cout << "[Server] " << clientKey(clientAddr)
                          << " is ready => " << ready << "/" << total << "\n";
                if (ready == total && total > 0 && !gameStarted) {
                    gameStarted = true;
                    broadcastSimple(sock, MSG_START, true, clients);
                    std::cout << "[Server] Game started!\n";
                }
                break;
            }
            case MSG_PLAYER_INPUT: {
                if (bytes >= (int)(sizeof(MessageHeader) + sizeof(PlayerInputPayload))) {
                    PlayerInputPayload inp;
                    std::memcpy(&inp, buffer + sizeof(MessageHeader), sizeof(PlayerInputPayload));

                    std::lock_guard<std::mutex> lock(stateMutex);
                    // If new player
                    if (!playerStates.count(inp.netID)) {
                        playerStates[inp.netID] = {400.f, 300.f, 3};
                        std::cout << "[Server] Create player netID=" << inp.netID << "\n";
                    }
                    playerInputs[inp.netID] = inp;
                }
                break;
            }
            case MSG_PING: {
                // bounce back MSG_PONG
                if (bytes >= (int)(sizeof(MessageHeader) + sizeof(PingPayload))) {
                    PingPayload pp;
                    std::memcpy(&pp, buffer + sizeof(MessageHeader), sizeof(PingPayload));

                    MessageHeader outH;
                    outH.type = MSG_PONG;
                    outH.sequence = 0;
                    outH.timestamp = getCurrentTimeMS();
                    outH.flags = 0;

                    char outPacket[sizeof(MessageHeader) + sizeof(PingPayload)];
                    std::memcpy(outPacket, &outH, sizeof(MessageHeader));
                    std::memcpy(outPacket + sizeof(MessageHeader), &pp, sizeof(PingPayload));

                    sendto(sock, outPacket, sizeof(outPacket), 0,
                           (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                }
                break;
            }
            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = std::atoi(argv[1]);

#ifdef _WIN32
    WSADATA wsa;
    int ws = WSAStartup(MAKEWORD(2,2), &wsa);
    if (ws != 0) {
        std::cerr << "WSAStartup failed: " << ws << "\n";
        return 1;
    }
#endif

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[Server] socket creation failed.\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[Server] bind failed.\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }
    std::cout << "[Server] Listening on port " << port << "\n";

    std::vector<sockaddr_in> clients;
    std::unordered_set<std::string> readySet;
    bool gameStarted = false;

    // Thread for receiving client packets
    std::thread clientThread(handleClient, sock,
                             std::ref(clients), std::ref(readySet),
                             std::ref(gameStarted));

    // Thread for broadcast lobby | until start
    std::thread lobbyThread([&](){
        while (!gameStarted) {
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                broadcastLobbyStatus(sock, clients,
                                     (uint8_t)clients.size(),
                                     (uint8_t)readySet.size());
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    // Main simulation thread
    std::thread simThread(simulationLoop, sock,
                          std::ref(clients),
                          std::ref(gameStarted));

    clientThread.join();
    simThread.join();
    lobbyThread.join();

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 0;
}
