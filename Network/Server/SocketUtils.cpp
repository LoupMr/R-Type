#include "SocketUtils.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdlib>

int initializeSocket(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[Server] Socket creation failed.\n";
        exit(1);
    }
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "[Server] Bind failed.\n";
        close(sock);
        exit(1);
    }
    std::cout << "[Server] Listening on port " << port << "\n";
    return sock;
}
