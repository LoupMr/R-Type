#include "ClientHandler.hpp"
#include "NetworkUtils.hpp"
#include "Globals.hpp"
#include "Network/Protocol/Protocol.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

void handleClientMessages(int sock, std::vector<sockaddr_in> &clients, IGame* game) {
    char buffer[1024];
    while (true) {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0,
                             reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);
        if (bytes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (bytes < static_cast<int>(sizeof(MessageHeader)))
            continue;

        MessageHeader inHeader;
        std::memcpy(&inHeader, buffer, sizeof(MessageHeader));

        if (inHeader.flags & 1)
            sendAck(sock, clientAddr, inHeader.sequence);

        std::string key = clientKey(clientAddr);
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
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
            case static_cast<uint8_t>(MessageType::READY): {
                std::cout << "[Server] Client " << key << " is ready.\n";
                {
                    std::lock_guard<std::mutex> lock(lobbyMutex);
                    lobbyStatus[key] = true;
                }
                break;
            }
            case static_cast<uint8_t>(MessageType::PLAYER_INPUT): {
                if (bytes >= static_cast<int>(sizeof(MessageHeader) + sizeof(PlayerInputPayload))) {
                    PlayerInputPayload input;
                    std::memcpy(&input, buffer + sizeof(MessageHeader), sizeof(PlayerInputPayload));
                    std::lock_guard<std::mutex> lock(pluginMutex);
                    game->onPlayerInput(input);
                }
                break;
            }
            case static_cast<uint8_t>(MessageType::PING): {
                if (bytes >= static_cast<int>(sizeof(MessageHeader) + sizeof(PingPayload))) {
                    PingPayload pp;
                    std::memcpy(&pp, buffer + sizeof(MessageHeader), sizeof(PingPayload));

                    MessageHeader outH;
                    outH.type = static_cast<uint8_t>(MessageType::PONG);
                    outH.sequence = 0;
                    outH.timestamp = getCurrentTimeMS();
                    outH.flags = 0;

                    char outPacket[sizeof(MessageHeader) + sizeof(PingPayload)];
                    std::memcpy(outPacket, &outH, sizeof(MessageHeader));
                    std::memcpy(outPacket + sizeof(MessageHeader), &pp, sizeof(PingPayload));

                    sendto(sock, outPacket, sizeof(outPacket), 0,
                           reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr));
                }
                break;
            }
            default:
                break;
        }
    }
}
