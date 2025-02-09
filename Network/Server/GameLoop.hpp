#ifndef GAME_LOOP_HPP
#define GAME_LOOP_HPP

#include "Game/IGame.hpp"
#include <vector>
#include <netinet/in.h>

void runGameLoop(int sock, IGame* game, std::vector<sockaddr_in>& clients);

#endif // GAME_LOOP_HPP
