#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <raylib.h>
#include <vector>

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
    float shootCooldown{2.0f}; // Time between shots
    float shootTimer{0.0f};    // Current cooldown timer
};

// Health component
struct Health {
    int current{3};
    int max{3};
};

#endif // COMPONENTS_HPP