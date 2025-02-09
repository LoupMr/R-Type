#ifndef RTYPE_TYPES_HPP
#define RTYPE_TYPES_HPP

#include <cstdint>
#include <vector>

enum class EnemyType {
    Normal,
    Fast,
    Strong
};

struct Player {
    int32_t id;
    float x;
    float y;
    int32_t health;
};

struct Enemy {
    int32_t enemyID;
    float x, y;
    float baseY;
    float vx;
    int32_t health;
    bool active;
    float shootTimer;
    EnemyType type;
    float patternTimer;
};

struct Bullet {
    int32_t bulletID;
    float x, y;
    float vx, vy;
    int32_t ownerID;
    bool active;
};

#endif // RTYPE_TYPES_HPP
