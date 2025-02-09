#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <dlfcn.h>

#include "Server/Globals.hpp"
#include "Server/NetworkUtils.hpp"
#include "Server/ClientHandler.hpp"
#include "Server/SocketUtils.hpp"
#include "Server/PluginLoader.hpp"
#include "Server/GameLoop.hpp"
#include "Game/IGame.hpp"
#include "Protocol/Protocol.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = std::atoi(argv[1]);
    int sock = initializeSocket(port);

    void* pluginHandle = nullptr;
    IGame* game = selectAndLoadPlugin(sock, &pluginHandle);

    std::vector<sockaddr_in> clients;
    std::thread clientThread(handleClientMessages, sock, std::ref(clients), game);

    runGameLoop(sock, game, clients);

    delete game;
    dlclose(pluginHandle);
    close(sock);
    clientThread.join();
    return 0;
}
