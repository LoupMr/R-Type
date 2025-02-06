#ifndef IGAME_HPP
#define IGAME_HPP

#include "Network/Protocol.hpp"  // Ensure that Protocol.hpp is in your include path

// A wrapper for the game state. Here we reuse GameStatePayload defined in Protocol.hpp.
struct GameState {
    GameStatePayload payload;
};

class IGame {
public:
    virtual ~IGame() = default;

    // Called once when the game starts (for initialization).
    virtual void onStart() = 0;

    // Called each time a client sends input.
    virtual void onPlayerInput(const PlayerInputPayload& input) = 0;

    // Called periodically (with delta time) to update the game simulation.
    virtual void onUpdate(float dt) = 0;

    // Returns the current game state to be broadcast to clients.
    virtual GameState getGameState() = 0;
};

extern "C" {
    // Factory function type that the shared library must export.
    typedef IGame* createGame_t();
}

#endif // IGAME_HPP
