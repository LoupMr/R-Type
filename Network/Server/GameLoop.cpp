#include "GameLoop.hpp"
#include "Globals.hpp"
#include "NetworkUtils.hpp"
#include "../Protocol/Protocol.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>
#include <cstring>

void runGameLoop(int sock, IGame* game, std::vector<sockaddr_in>& clients) {
    auto lastTime = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::lock_guard<std::mutex> lock2(lobbyMutex);
            uint8_t total = static_cast<uint8_t>(clients.size());
            uint8_t ready = 0;
            for (const auto &entry : lobbyStatus) {
                if (entry.second)
                    ready++;
            }
            broadcastLobbyStatus(sock, clients, total, ready);

            if (total > 0 && ready == total && !gameStarted) {
                std::cout << "[Server] All clients ready. Starting game.\n";
                {
                    std::lock_guard<std::mutex> lock(pluginMutex);
                    game->onStart();
                }
                MessageHeader startHeader;
                startHeader.type      = static_cast<uint8_t>(MessageType::START);
                startHeader.sequence  = 0;
                startHeader.timestamp = getCurrentTimeMS();
                startHeader.flags     = 1; // Mark as important
                char startPacket[sizeof(MessageHeader)];
                std::memcpy(startPacket, &startHeader, sizeof(MessageHeader));
                broadcastPacket(sock, startPacket, sizeof(startPacket), clients);
                gameStarted = true;
            }
        }

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
}
