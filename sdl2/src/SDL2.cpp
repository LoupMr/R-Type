#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <cmath>
#include "ECS.hpp"

// ----------------
// 1) SDL2 Components
// ----------------

// Basic position and velocity (library-agnostic themselves)
struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

// For rendering
struct Sprite {
    SDL_Texture* texture;
    int w, h; // render dimensions
};

// For input
struct KeyboardControl {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
};

// ----------------
// 2) SDL2 Systems
// ----------------

// Renders any entity with Position + Sprite
class RenderSystem : public System {
public:
    RenderSystem(SDL_Renderer* renderer) : m_renderer(renderer) {}
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        (void)dt; // Not used here, purely rendering

        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto pos = cm.getComponent<Position>(e);
            auto spr = cm.getComponent<Sprite>(e);
            if (pos && spr && spr->texture) {
                SDL_Rect dst;
                dst.x = static_cast<int>(pos->x);
                dst.y = static_cast<int>(pos->y);
                dst.w = spr->w;
                dst.h = spr->h;
                SDL_RenderCopy(m_renderer, spr->texture, nullptr, &dst);
            }
        }
    }
private:
    SDL_Renderer* m_renderer;
};

// Moves any entity with Position + Velocity
class MovementSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto pos = cm.getComponent<Position>(e);
            auto vel = cm.getComponent<Velocity>(e);
            if (pos && vel) {
                pos->x += vel->vx * dt;
                pos->y += vel->vy * dt;
            }
        }
    }
};

// Keyboard input system that updates KeyboardControl + Velocity
class InputSystem : public System {
public:
    void handleSDLEvent(const SDL_Event& e, EntityManager& em, ComponentManager& cm) {
        // For each entity that has KeyboardControl, set booleans
        for (auto entity : em.getAllEntities()) {
            auto kb = cm.getComponent<KeyboardControl>(entity);
            if (!kb) continue;

            if (e.type == SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_w: kb->up    = true; break;
                    case SDLK_s: kb->down  = true; break;
                    case SDLK_a: kb->left  = true; break;
                    case SDLK_d: kb->right = true; break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch(e.key.keysym.sym) {
                    case SDLK_w: kb->up    = false; break;
                    case SDLK_s: kb->down  = false; break;
                    case SDLK_a: kb->left  = false; break;
                    case SDLK_d: kb->right = false; break;
                }
            }
        }
    }

    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        // For demonstration, let's set velocity based on keyboard booleans
        for (auto entity : em.getAllEntities()) {
            auto kb  = cm.getComponent<KeyboardControl>(entity);
            auto vel = cm.getComponent<Velocity>(entity);
            if (kb && vel) {
                vel->vx = 0.f;
                vel->vy = 0.f;
                if (kb->up)    vel->vy = -100.f;
                if (kb->down)  vel->vy =  100.f;
                if (kb->left)  vel->vx = -100.f;
                if (kb->right) vel->vx =  100.f;
            }
        }
    }
};

// AudioSystem that plays a sound if velocity is large
class AudioSystem : public System {
public:
    AudioSystem() {
        // Initialize audio
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
            std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << "\n";
        }
        // Load a sample sound
        m_sound = Mix_LoadWAV("beep.wav");
        if (!m_sound) {
            std::cerr << "Failed to load beep.wav\n";
        }
    }
    ~AudioSystem() {
        if (m_sound) Mix_FreeChunk(m_sound);
        Mix_CloseAudio();
    }

    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        (void)dt;
        // Example: if velocity magnitude is big => beep
        for (auto e : em.getAllEntities()) {
            auto vel = cm.getComponent<Velocity>(e);
            if (vel) {
                float speed = std::sqrt(vel->vx * vel->vx + vel->vy * vel->vy);
                if (speed > 100.0f && m_sound) {
                    Mix_PlayChannel(-1, m_sound, 0);
                }
            }
        }
    }
private:
    Mix_Chunk* m_sound = nullptr;
};
