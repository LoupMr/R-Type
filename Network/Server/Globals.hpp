#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <mutex>
#include <unordered_map>
#include <string>
#include <netinet/in.h>

extern std::mutex clientsMutex;
extern std::mutex pluginMutex;
extern std::mutex lobbyMutex;

extern std::unordered_map<std::string, bool> lobbyStatus;
extern bool gameStarted;

#endif // GLOBALS_HPP
