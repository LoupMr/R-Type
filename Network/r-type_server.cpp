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
    #include <dlfcn.h>  // For dlopen, dlsym, dlclose
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <unordered_map>
namespace fs = std::filesystem;

#include "Protocol.hpp"
#include "Game/IGame.hpp"  // Adjust include path if needed

// Global mutexes and lobby status.
std::mutex clientsMutex;
std::mutex pluginMutex;
std::mutex lobbyMutex;
std::unordered_map<std::string, bool> lobbyStatus;  // key: client key, value: ready?
bool gameStarted = false;

// Utility: Create a unique string for a client address.
std::string clientKey(const sockaddr_in &c) {
    char buf[64];
    // Use snprintf to avoid deprecated sprintf.
    snprintf(buf, sizeof(buf), "%s:%d", inet_ntoa(c.sin_addr), ntohs(c.sin_port));
    return std::string(buf);
}

// Sends an ACK message back to a client.
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

// Broadcasts a raw packet to all connected clients.
void broadcastPacket(int sock, const char* data, size_t len, const std::vector<sockaddr_in> &clients) {
    for (const auto &cl : clients) {
        int sent = sendto(sock, data, (int)len, 0, (struct sockaddr*)&cl, sizeof(cl));
        if (sent < 0) {
            std::cerr << "[Server] broadcastPacket failed.\n";
        }
    }
}

// Sends a lobby status message (MSG_LOBBY_STATUS) to all clients.
void broadcastLobbyStatus(int sock, const std::vector<sockaddr_in> &clients, uint8_t total, uint8_t ready) {
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

// Wraps the game state (GameStatePayload) into a network message and broadcasts it.
void broadcastGameState(int sock, const std::vector<sockaddr_in> &clients, const GameStatePayload &gsPayload) {
    MessageHeader mh;
    mh.type      = MSG_GAME_STATE;
    mh.sequence  = 0;
    mh.timestamp = getCurrentTimeMS();
    mh.flags     = 1; // Mark as important

    char packet[sizeof(MessageHeader) + sizeof(GameStatePayload)];
    std::memcpy(packet, &mh, sizeof(MessageHeader));
    std::memcpy(packet + sizeof(MessageHeader), &gsPayload, sizeof(GameStatePayload));

    broadcastPacket(sock, packet, sizeof(packet), clients);
}

// Thread function to handle incoming messages from clients.
void handleClientMessages(int sock, std::vector<sockaddr_in> &clients, IGame* game) {
    char buffer[1024];
    while (true) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif
        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (bytes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (bytes < (int)sizeof(MessageHeader))
            continue;

        MessageHeader inHeader;
        std::memcpy(&inHeader, buffer, sizeof(MessageHeader));

        // If the message is marked important, send an ACK.
        if (inHeader.flags & 1)
            sendAck(sock, clientAddr, inHeader.sequence);

        std::string key = clientKey(clientAddr);
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            // Add the client if not already present.
            bool exists = false;
            for (const auto &c : clients) {
                if (c.sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
                    c.sin_port == clientAddr.sin_port) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                clients.push_back(clientAddr);
                std::cout << "[Server] New client: " << key << "\n";
            }
        }

        switch (inHeader.type) {
            case MSG_READY: {
                std::cout << "[Server] Client " << key << " is ready.\n";
                {
                    std::lock_guard<std::mutex> lock(lobbyMutex);
                    lobbyStatus[key] = true;
                }
                // Do not immediately broadcast MSG_START.
                break;
            }
            case MSG_PLAYER_INPUT: {
                if (bytes >= (int)(sizeof(MessageHeader) + sizeof(PlayerInputPayload))) {
                    PlayerInputPayload input;
                    std::memcpy(&input, buffer + sizeof(MessageHeader), sizeof(PlayerInputPayload));
                    std::lock_guard<std::mutex> lock(pluginMutex);
                    game->onPlayerInput(input);
                }
                break;
            }
            case MSG_PING: {
                // Bounce back a PONG message for latency measurement.
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

                    sendto(sock, outPacket, sizeof(outPacket), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                }
                break;
            }
            default:
                // Ignore other message types.
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    // Expect only the port as a command-line parameter.
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
        std::cerr << "[Server] Socket creation failed.\n";
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
        std::cerr << "[Server] Bind failed.\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }
    std::cout << "[Server] Listening on port " << port << "\n";

    // -----------------------------
    // Plugin selection via menu:
    // -----------------------------
    std::vector<std::string> pluginFiles;
    // Change pluginsDir to the folder where your plugin shared libraries are stored.
    std::string pluginsDir = "./Game";
    if (!fs::exists(pluginsDir)) {
        std::cerr << "Plugins directory '" << pluginsDir << "' does not exist.\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    for (const auto & entry : fs::directory_iterator(pluginsDir)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
#ifdef _WIN32
            if (path.extension() == ".dll")
#elif defined(__APPLE__)
            if (path.extension() == ".dylib")
#else
            if (path.extension() == ".so")
#endif
            {
                pluginFiles.push_back(path.string());
            }
        }
    }

    if (pluginFiles.empty()) {
        std::cerr << "No plugins found in " << pluginsDir << "\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    std::cout << "Available game plugins:\n";
    for (size_t i = 0; i < pluginFiles.size(); i++) {
        std::cout << i << ": " << pluginFiles[i] << "\n";
    }
    std::cout << "Select a plugin by number: ";
    int choice;
    std::cin >> choice;
    if (choice < 0 || choice >= static_cast<int>(pluginFiles.size())) {
        std::cerr << "Invalid plugin choice.\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }
    std::string selectedPlugin = pluginFiles[choice];
    std::cout << "Loading plugin: " << selectedPlugin << "\n";

    // -----------------------------
    // Load the selected plugin:
    // -----------------------------
    void* pluginHandle = dlopen(selectedPlugin.c_str(), RTLD_LAZY);
    if (!pluginHandle) {
        std::cerr << "[Server] Failed to load plugin: " << dlerror() << "\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }
    dlerror(); // Clear any existing error.

    // Obtain the factory function to create the game instance.
    createGame_t* createGame = (createGame_t*) dlsym(pluginHandle, "createGame");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "[Server] Cannot load symbol 'createGame': " << dlsym_error << "\n";
        dlclose(pluginHandle);
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    IGame* game = createGame();
    if (!game) {
        std::cerr << "[Server] Failed to create game instance.\n";
        dlclose(pluginHandle);
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return 1;
    }

    // -----------------------------
    // Start the client message handler thread.
    // -----------------------------
    std::vector<sockaddr_in> clients;
    std::thread clientThread(handleClientMessages, sock, std::ref(clients), game);

    // Main loop: update the game simulation, broadcast lobby status, and (if all clients are ready) start the game.
    auto lastTime = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // Broadcast lobby status.
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::lock_guard<std::mutex> lock2(lobbyMutex);
            uint8_t total = static_cast<uint8_t>(clients.size());
            uint8_t ready = 0;
            for (const auto &entry : lobbyStatus) {
                if (entry.second) ready++;
            }
            broadcastLobbyStatus(sock, clients, total, ready);

            // If all clients are ready and the game has not yet started, then start the game.
            if (total > 0 && ready == total && !gameStarted) {
                std::cout << "[Server] All clients ready. Starting game.\n";
                {
                    std::lock_guard<std::mutex> lock(pluginMutex);
                    game->onStart();
                }
                // Broadcast a MSG_START message.
                MessageHeader startHeader;
                startHeader.type = MSG_START;
                startHeader.sequence = 0;
                startHeader.timestamp = getCurrentTimeMS();
                startHeader.flags = 1; // Mark as important
                char startPacket[sizeof(MessageHeader)];
                std::memcpy(startPacket, &startHeader, sizeof(MessageHeader));
                broadcastPacket(sock, startPacket, sizeof(startPacket), clients);
                gameStarted = true;
            }
        }

        // If the game has started, update the simulation and broadcast the game state.
        if (gameStarted) {
            std::lock_guard<std::mutex> lock(pluginMutex);
            game->onUpdate(dt);
            GameState state = game->getGameState();
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                broadcastGameState(sock, clients, state.payload);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup (this part is never reached because of the infinite loop).
    delete game;
    dlclose(pluginHandle);
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    clientThread.join();
    return 0;
}
