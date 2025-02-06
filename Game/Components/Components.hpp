#ifndef GAME_COMPONENTS_HPP
#define GAME_COMPONENTS_HPP

#include <raylib.h>

// Position component
struct Position {
    float x, y;
};

// Velocity component
struct Velocity {
    float vx, vy;
};

// Sprite component
struct Sprite {
    Texture2D texture;
    int w, h;
};

// Keyboard control component
struct KeyboardControl {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool shoot{false};
};

// Bullet component
struct Bullet {
    bool isActive{true};
    float speed{500.0f};
    float maxDistance{800.0f};
    float distanceTraveled{0.0f};
};

// Enemy component
struct Enemy {
    float speed{100.0f};
    int health{3};
    float shootCooldown{2.0f};
    float shootTimer{0.0f};
};

// Health component
struct Health {
    int current{3};
    int max{3};
};

#endif // GAME_COMPONENTS_HPP
