#include "PluginLoader.hpp"
#include <filesystem>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <dlfcn.h>
#include <unistd.h>

namespace fs = std::filesystem;

IGame* selectAndLoadPlugin(int sock, void** pluginHandle) {
    std::vector<std::string> pluginFiles;
    std::string pluginsDir = "./Game";
    if (!fs::exists(pluginsDir)) {
        std::cerr << "Plugins directory '" << pluginsDir << "' does not exist.\n";
        close(sock);
        exit(1);
    }

    for (const auto & entry : fs::directory_iterator(pluginsDir)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
    #if defined(__APPLE__)
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
        close(sock);
        exit(1);
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
        close(sock);
        exit(1);
    }
    std::string selectedPlugin = pluginFiles[choice];
    std::cout << "Loading plugin: " << selectedPlugin << "\n";

    *pluginHandle = dlopen(selectedPlugin.c_str(), RTLD_LAZY);
    if (!*pluginHandle) {
        std::cerr << "[Server] Failed to load plugin: " << dlerror() << "\n";
        close(sock);
        exit(1);
    }
    dlerror(); // Clear any existing error.

    typedef IGame* (*createGame_t)();
    createGame_t createGame = (createGame_t)dlsym(*pluginHandle, "createGame");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "[Server] Cannot load symbol 'createGame': " << dlsym_error << "\n";
        dlclose(*pluginHandle);
        close(sock);
        exit(1);
    }

    IGame* game = createGame();
    if (!game) {
        std::cerr << "[Server] Failed to create game instance.\n";
        dlclose(*pluginHandle);
        close(sock);
        exit(1);
    }
    return game;
}
