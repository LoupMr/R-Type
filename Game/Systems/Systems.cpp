#include "Systems.hpp"
#include "Engine/Graphics/Audio.hpp"
#include "Engine/Graphics/Window.hpp"
#include "Engine/Graphics/Texture.hpp"
#include "Engine/Graphics/Input.hpp"
#include <cmath>
#include <iostream>

// RenderSystem
void RenderSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto pos = cm.getComponent<Position>(e);
        auto spr = cm.getComponent<Sprite>(e);
        if (pos && spr) {
            Vector2 position = {pos->x, pos->y};
            DrawTextureV(spr->texture, position, WHITE);
        }
    }

    // Display player health (assuming local player is entity 0)
    Engine::Entity player = 0;
    auto health = cm.getComponent<Health>(player);
    if (health) {
        DrawText(TextFormat("Health: %d/%d", health->current, health->max), 10, 10, 20, RED);
    }
}

// MovementSystem
void MovementSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto pos = cm.getComponent<Position>(e);
        auto vel = cm.getComponent<Velocity>(e);
        if (pos && vel) {
            pos->x += vel->vx * dt;
            pos->y += vel->vy * dt;

            // Example bullet logic: destroy bullets if they go off-screen
            if (pos->x < 0 || pos->x > 800 || pos->y < 0 || pos->y > 600) {
                em.destroyEntity(e);
            }
        }
    }
}

// InputSystem
void InputSystem::handleInput(Engine::EntityManager& em, Engine::ComponentManager& cm) {
    for (auto entity : em.getAllEntities()) {
        auto kb = cm.getComponent<KeyboardControl>(entity);
        if (!kb) continue;

        // Using Engine::Input for keystates
        kb->up    = Engine::Input::IsKeyDown(KEY_W);
        kb->down  = Engine::Input::IsKeyDown(KEY_S);
        kb->left  = Engine::Input::IsKeyDown(KEY_A);
        kb->right = Engine::Input::IsKeyDown(KEY_D);
        kb->shoot = Engine::Input::IsKeyPressed(KEY_SPACE);
    }
}

void InputSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    for (auto entity : em.getAllEntities()) {
        auto kb = cm.getComponent<KeyboardControl>(entity);
        auto vel = cm.getComponent<Velocity>(entity);
        if (kb && vel) {
            vel->vx = 0.f;
            vel->vy = 0.f;
            if (kb->up)    vel->vy = -200.f;
            if (kb->down)  vel->vy = 200.f;
            if (kb->left)  vel->vx = -200.f;
            if (kb->right) vel->vx = 200.f;
        }
    }
}

// ShootingSystem
void ShootingSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto kb = cm.getComponent<KeyboardControl>(e);
        auto pos = cm.getComponent<Position>(e);
        if (kb && kb->shoot && pos) {
            // Create a bullet
            Engine::Entity bullet = em.createEntity();
            cm.addComponent(bullet, Position{pos->x + 50, pos->y});
            cm.addComponent(bullet, Velocity{500.0f, 0.0f});
            cm.addComponent(bullet, Bullet{});
            cm.addComponent(bullet, Sprite{cm.getGlobalTexture("bullet"), 10, 10});
        }
    }
}

// EnemySystem
void EnemySystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    static float spawnTimer = 0.0f;
    spawnTimer += dt;

    if (spawnTimer >= 2.0f) { // spawn an enemy every 2 seconds
        spawnTimer = 0.0f;
        std::cout << "[DEBUG] Spawning enemy..." << std::endl;

        Engine::Entity enemy = em.createEntity();
        cm.addComponent(enemy, Position{800.0f, static_cast<float>(GetRandomValue(50, 550))});
        cm.addComponent(enemy, Velocity{-100.0f, 0.0f});
        cm.addComponent(enemy, Enemy{});
        cm.addComponent(enemy, Sprite{cm.getGlobalTexture("enemy"), 50, 50});
    }

    // Basic enemy shooting
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;
        auto en = cm.getComponent<Enemy>(e);
        if (en) {
            en->shootTimer += dt;
            if (en->shootTimer >= en->shootCooldown) {
                en->shootTimer = 0.0f;
                auto pos = cm.getComponent<Position>(e);
                if (pos) {
                    Engine::Entity bullet = em.createEntity();
                    cm.addComponent(bullet, Position{pos->x - 20, pos->y});
                    cm.addComponent(bullet, Velocity{-500.0f, 0.0f});
                    cm.addComponent(bullet, Bullet{});
                    cm.addComponent(bullet, Sprite{cm.getGlobalTexture("bullet"), 10, 10});
                }
            }
        }
    }
}

// CollisionSystem
void CollisionSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    // Very naive collision logic for bullets & enemies/players
    Engine::Entity player = 0;
    auto playerHealth = cm.getComponent<Health>(player);

    // Check if player is dead => respawn logic
    if (playerHealth && playerHealth->current <= 0) {
        em.destroyEntity(player);
        playerDead = true;
        respawnTimer = 3.0f;
    }
    if (playerDead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0) {
            // Respawn
            player = em.createEntity();
            cm.addComponent(player, Position{100.0f, 300.0f});
            cm.addComponent(player, Velocity{0.0f, 0.0f});
            cm.addComponent(player, KeyboardControl{});
            cm.addComponent(player, Health{3,3});
            cm.addComponent(player, Sprite{cm.getGlobalTexture("player"), 50, 50});
            playerDead = false;
        }
    }

    // Bullet collisions
    for (auto b : em.getAllEntities()) {
        if (!em.isAlive(b)) continue;
        auto bPos = cm.getComponent<Position>(b);
        auto bComp = cm.getComponent<Bullet>(b);
        if (!bPos || !bComp) continue;

        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e) || e == b) continue;

            auto en = cm.getComponent<Enemy>(e);
            auto hp = cm.getComponent<Health>(e);
            auto pos = cm.getComponent<Position>(e);

            if (pos && (en || hp)) {
                float dx = bPos->x - pos->x;
                float dy = bPos->y - pos->y;
                float dist = sqrtf(dx*dx + dy*dy);

                if (dist < 20.0f) {
                    if (en) {
                        en->health--;
                        if (en->health <= 0) {
                            em.destroyEntity(e);
                        }
                    } else if (hp) {
                        hp->current--;
                        if (hp->current <= 0) {
                            em.destroyEntity(e); // i.e. kill the player
                        }
                    }
                    em.destroyEntity(b);
                    break;
                }
            }
        }
    }
}

// AudioSystem
AudioSystem::AudioSystem(Sound sound) : m_sound(sound) {}
AudioSystem::~AudioSystem() {}

void AudioSystem::update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) {
    // Example: play sound if velocity is high
    for (auto e : em.getAllEntities()) {
        auto vel = cm.getComponent<Velocity>(e);
        if (vel) {
            float speed = std::sqrt(vel->vx * vel->vx + vel->vy * vel->vy);
            if (speed >= 100.f && m_sound.frameCount > 0) {
                PlaySound(m_sound);
            }
        }
    }
}
