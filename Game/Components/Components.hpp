#ifndef GAME_COMPONENTS_HPP
#define GAME_COMPONENTS_HPP

#include <raylib.h>

struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

struct Sprite {
    Texture2D texture;
    int w, h;
};

struct KeyboardControl {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool shoot{false};
};

struct Bullet {
    bool isActive{true};
    float speed{500.0f};
    float maxDistance{800.0f};
    float distanceTraveled{0.0f};
};

struct Enemy {
    float speed{100.0f};
    int health{3};
    float shootCooldown{2.0f};
    float shootTimer{0.0f};
};

struct Health {
    int current{3};
    int max{3};
};

#endif // GAME_COMPONENTS_HPP