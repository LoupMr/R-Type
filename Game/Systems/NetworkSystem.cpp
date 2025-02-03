#include "NetworkSystem.hpp"
#include <iostream>
#include <sstream>
#include <cstring>  // For memset

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

NetworkSystem::NetworkSystem(const std::string& serverIP, int serverPort, int clientPort) {
#ifdef _WIN32
    WSADATA wsaData;
    int wsResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(wsResult != 0) {
        std::cerr << "[NetworkSystem] WSAStartup failed: " << wsResult << "\n";
    }
#endif

    // Create a UDP socket.
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[NetworkSystem] Error creating socket.\n";
    }

    // Bind the socket to the given client port.
    struct sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    clientAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr)) < 0) {
        std::cerr << "[NetworkSystem] Bind failed.\n";
    }

    // Set up the server address.
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
#ifdef _WIN32
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);
#else
    inet_aton(serverIP.c_str(), &serverAddr.sin_addr);
#endif

    std::cout << "[NetworkSystem] Initialized. Sending to " 
              << serverIP << ":" << serverPort
              << ", bound to client port " << clientPort << "\n";
}

NetworkSystem::~NetworkSystem() {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

bool NetworkSystem::sendData(const std::string &data) {
    int sentBytes = sendto(sock, data.c_str(), static_cast<int>(data.size()), 0,
                           reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (sentBytes < 0) {
        std::cerr << "[NetworkSystem] sendto failed.\n";
        return false;
    }
    return true;
}

void NetworkSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    // Iterate over all entities in the ECS.
    auto entities = em.getAllEntities();
    for (auto entity : entities) {
        if (!em.isAlive(entity)) continue;  // Skip dead entities

        // Check if the entity has a Position component.
        Position* pos = cm.getComponent<Position>(entity);
        if (pos) {
            // Construct a message string.
            std::ostringstream oss;
            oss << "ENTITY " << entity << " POS " << pos->x << " " << pos->y;
            std::string message = oss.str();

            // Send the message to the server.
            if (sendData(message)) {
                std::cout << "[NetworkSystem] Sent: " << message << "\n";
            }
        }
    }
}
