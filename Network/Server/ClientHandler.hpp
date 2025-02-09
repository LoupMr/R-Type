#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <netinet/in.h>
#include <vector>
#include "Game/IGame.hpp"

void handleClientMessages(int sock, std::vector<sockaddr_in> &clients, IGame* game);

#endif // CLIENT_HANDLER_HPP
