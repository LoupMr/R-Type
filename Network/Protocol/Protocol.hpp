#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <cstring>
#include <chrono>

#pragma pack(push, 1)

enum class MessageType : uint8_t {
    READY         = 1,
    START         = 2,
    ACK           = 5,
    PING          = 6,
    PONG          = 7,
    GAME_STATE    = 8,
    LOBBY_STATUS  = 10,
    PLAYER_INPUT  = 11
};

struct MessageHeader {
    uint8_t  type;
    uint32_t sequence;
    uint32_t timestamp;
    uint8_t  flags;
};

struct AckPayload {
    uint32_t ackSequence;
};

struct PingPayload {
    uint32_t pingSequence;
};

struct LobbyStatusPayload {
    uint8_t totalClients;
    uint8_t readyClients;
};

struct PlayerInputPayload {
    int32_t netID;
    bool up;
    bool down;
    bool left;
    bool right;
    bool shoot;
};

struct BulletState {
    int32_t bulletID;
    float   x;
    float   y;
    float   vx;
    float   vy;
    int32_t ownerID;
};

struct EnemyState {
    int32_t enemyID;
    float   x;
    float   y;
    int32_t health;
    uint8_t type;
};

struct GameStatePayload {
    uint8_t numEnemies;
    EnemyState enemies[32];

    uint8_t numPlayers;
    struct PlayerState {
        int32_t playerID;
        float   x;
        float   y;
        int32_t health;
    } players[4];

    uint8_t numBullets;
    BulletState bullets[32];
};

#pragma pack(pop)

inline uint32_t getCurrentTimeMS() {
    using ms = std::chrono::milliseconds;
    return static_cast<uint32_t>(
        std::chrono::duration_cast<ms>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

#endif // PROTOCOL_HPP
