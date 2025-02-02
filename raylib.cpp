#include <raylib.h>
#include <iostream>
#include <cmath>

// If you have your own ECS headers in different folders, update the paths here:
#include "../../ecs/include/EntityManager.hpp"
#include "../../ecs/include/Components/ComponentManager.hpp"
#include "../../ecs/include/Systems/SystemManager.hpp"

// --------------------------------------------------------------------------------
// Simple components (Position, Velocity, Sprite, KeyboardControl)
// --------------------------------------------------------------------------------

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
};

// --------------------------------------------------------------------------------
// Helper function to construct asset paths
// --------------------------------------------------------------------------------
std::string GetAssetPath(const std::string &assetName) {
    // Adjust if needed. Make sure "assets" folder is in your working directory.
    return "../assets/" + assetName;
}

// --------------------------------------------------------------------------------
// Systems
// --------------------------------------------------------------------------------
class RenderSystem : public System {
public:
    void update(float dt, EntityManager &em, ComponentManager &cm) override {
        (void)dt;
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e))
                continue;

            auto pos = cm.getComponent<Position>(e);
            auto spr = cm.getComponent<Sprite>(e);
            if (pos && spr) {
                // Draw only if valid texture
                if (spr->texture.id != 0) {
                    Vector2 position = {pos->x, pos->y};
                    DrawTextureV(spr->texture, position, WHITE);
                }
            }
        }
    }
};

class MovementSystem : public System {
public:
    void update(float dt, EntityManager &em, ComponentManager &cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e))
                continue;

            auto pos = cm.getComponent<Position>(e);
            auto vel = cm.getComponent<Velocity>(e);
            if (pos && vel) {
                pos->x += vel->vx * dt;
                pos->y += vel->vy * dt;
            }
        }
    }
};

class InputSystem : public System {
public:
    void handleInput(EntityManager &em, ComponentManager &cm) {
        for (auto entity : em.getAllEntities()) {
            auto kb = cm.getComponent<KeyboardControl>(entity);
            if (!kb)
                continue;
            // WASD controls
            kb->up    = IsKeyDown(KEY_W);
            kb->down  = IsKeyDown(KEY_S);
            kb->left  = IsKeyDown(KEY_A);
            kb->right = IsKeyDown(KEY_D);
        }
    }

    void update(float dt, EntityManager &em, ComponentManager &cm) override {
        (void)dt;
        for (auto entity : em.getAllEntities()) {
            auto kb  = cm.getComponent<KeyboardControl>(entity);
            auto vel = cm.getComponent<Velocity>(entity);
            if (kb && vel) {
                vel->vx = 0.f;
                vel->vy = 0.f;
                if (kb->up)
                    vel->vy = -100.f;
                if (kb->down)
                    vel->vy = 100.f;
                if (kb->left)
                    vel->vx = -100.f;
                if (kb->right)
                    vel->vx = 100.f;
            }
        }
    }
};

class AudioSystem : public System {
public:
    AudioSystem(Sound sound) : m_sound(sound) {}
    ~AudioSystem() {
        // We do NOT UnloadSound here, main() owns the sound resource
    }

    void update(float dt, EntityManager &em, ComponentManager &cm) override {
        (void)dt;
        for (auto e : em.getAllEntities()) {
            auto vel = cm.getComponent<Velocity>(e);
            if (vel) {
                float speed = std::sqrt(vel->vx * vel->vx + vel->vy * vel->vy);
                // Use >= to allow speed == 100 to trigger the beep
                if (speed >= 100.0f && m_sound.frameCount > 0) {
                    PlaySound(m_sound);
                }
            }
        }
    }

private:
    Sound m_sound;
};

// --------------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------------
int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Raylib ECS Example");
    SetTargetFPS(60);

    // Initialize audio device
    InitAudioDevice();
    Sound beepSound = LoadSound(GetAssetPath("beep.wav").c_str());
    if (beepSound.frameCount == 0) {
        std::cerr << "[ERROR] Failed to load beep.wav\n";
    } else {
        std::cout << "[INFO] beep.wav loaded successfully.\n";
    }

    // ECS-related managers
    EntityManager em;
    ComponentManager cm;

    // Create a player entity
    Entity player = em.createEntity();
    cm.addComponent(player, Position{400.0f, 300.0f});
    cm.addComponent(player, Velocity{0.0f, 0.0f});

    // Load the player texture
    Texture2D playerTexture = LoadTexture(GetAssetPath("player.png").c_str());
    if (playerTexture.id == 0) {
        std::cerr << "[ERROR] Failed to load player.png\n";
    } else {
        std::cout << "[INFO] player.png loaded successfully. ("
                  << playerTexture.width << "x" << playerTexture.height << ")\n";
    }
    cm.addComponent(player, Sprite{playerTexture, playerTexture.width, playerTexture.height});
    cm.addComponent(player, KeyboardControl{});

    // Create systems
    RenderSystem renderSystem;
    MovementSystem movementSystem;
    InputSystem inputSystem;
    AudioSystem audioSystem(beepSound);

    // Main loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1) Handle input
        inputSystem.handleInput(em, cm);

        // 2) Update systems
        inputSystem.update(dt, em, cm);
        movementSystem.update(dt, em, cm);
        audioSystem.update(dt, em, cm);

        // 3) Render
        BeginDrawing();
        ClearBackground(RAYWHITE);
        renderSystem.update(dt, em, cm);
        EndDrawing();
    }

    // Cleanup
    UnloadSound(beepSound);
    CloseAudioDevice();
    UnloadTexture(playerTexture);

    CloseWindow();
    return 0;
}
