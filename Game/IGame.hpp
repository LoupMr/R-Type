#ifndef IGAME_HPP
#define IGAME_HPP

#include "Network/Protocol/Protocol.hpp"

struct GameState {
    GameStatePayload payload{};
};

class IGame {
public:
    virtual ~IGame() = default;
    virtual void onStart() = 0;
    virtual void onPlayerInput(const PlayerInputPayload& input) = 0;
    virtual void onUpdate(float dt) = 0;
    virtual GameState getGameState() = 0;
};

extern "C" {
    IGame* createGame();
}

#endif // IGAME_HPP
