#ifndef PLUGIN_LOADER_HPP
#define PLUGIN_LOADER_HPP

#include "Game/IGame.hpp"

IGame* selectAndLoadPlugin(int sock, void** pluginHandle);

#endif // PLUGIN_LOADER_HPP
