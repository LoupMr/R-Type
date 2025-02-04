#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <cstring>
#include <chrono>

#pragma pack(push, 1)

// -------------------------------------
// Message types
// -------------------------------------
enum MessageType : uint8_t {
    MSG_READY         = 1,
    MSG_START         = 2,
    MSG_ACK           = 5,
    MSG_PING          = 6,
    MSG_PONG          = 7,
    MSG_GAME_STATE    = 8,
    MSG_LOBBY_STATUS  = 10,
    MSG_PLAYER_INPUT  = 11
};

// -------------------------------------
struct MessageHeader {
    uint8_t  type;       // which message (see enum)
    uint32_t sequence;   // sequence number
    uint32_t timestamp;  // e.g. ms from some clock
    uint8_t  flags;      // bit 0 => important
};

// For acknowledging important messages
struct AckPayload {
    uint32_t ackSequence;
};

// For latency measurement
struct PingPayload {
    uint32_t pingSequence;
};

// For showing lobby info
struct LobbyStatusPayload {
    uint8_t totalClients;
    uint8_t readyClients;
};

// From client: input (up/down/left/right/shoot)
struct PlayerInputPayload {
    int32_t netID;
    bool up;
    bool down;
    bool left;
    bool right;
    bool shoot;
};

// A bullet in the game state
struct BulletState {
    int32_t bulletID;
    float   x;
    float   y;
    float   vx;
    float   vy;
    int32_t ownerID; // >= 0 => player bullet, <0 => enemy bullet
};

// An enemy in the game state
struct EnemyState {
    int32_t enemyID;
    float   x;
    float   y;
    int32_t health;
};

// Full game state snapshot
struct GameStatePayload {
    // Enemies
    uint8_t numEnemies;
    EnemyState enemies[32];

    // Players
    uint8_t numPlayers;
    struct PlayerState {
        int32_t playerID;
        float   x;
        float   y;
        int32_t health;
    } players[4];

    // Bullets
    uint8_t numBullets;
    BulletState bullets[32];
};

#pragma pack(pop)

// Utility to get current time in ms (relative to steady_clock)
inline uint32_t getCurrentTimeMS() {
    using ms = std::chrono::milliseconds;
    return static_cast<uint32_t>(
        std::chrono::duration_cast<ms>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

#endif // PROTOCOL_HPP
