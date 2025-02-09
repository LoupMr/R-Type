#include "Globals.hpp"

std::mutex clientsMutex;
std::mutex pluginMutex;
std::mutex lobbyMutex;
std::unordered_map<std::string, bool> lobbyStatus;
bool gameStarted = false;
