#include "Systems/Systems.hpp"
#include <raylib.h>
#include <cmath>
#include <iostream>

// RenderSystem
void RenderSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto pos = cm.getComponent<Position>(e);
        auto spr = cm.getComponent<Sprite>(e);
        if (pos && spr) {
            Vector2 position = {pos->x, pos->y};
            DrawTextureV(spr->texture, position, WHITE);
        }
    }

    // Display player health
    Entity player = 0; // Assuming player entity ID is 0
    auto health = cm.getComponent<Health>(player);
    if (health) {
        DrawText(TextFormat("Health: %d/%d", health->current, health->max), 10, 10, 20, RED);
    }
}

// MovementSystem
void MovementSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto pos = cm.getComponent<Position>(e);
        auto vel = cm.getComponent<Velocity>(e);
        if (pos && vel) {
            pos->x += vel->vx * dt;
            pos->y += vel->vy * dt;

            // Update bullet distance
            // Destroy bullets if they move off-screen
            if (pos->x < 0 || pos->x > 800 || pos->y < 0 || pos->y > 600) {
                em.destroyEntity(e);
            }
        }
    }
}

// InputSystem
void InputSystem::handleInput(EntityManager& em, ComponentManager& cm) {
    for (auto entity : em.getAllEntities()) {
        auto kb = cm.getComponent<KeyboardControl>(entity);
        if (!kb) continue;

        kb->up = IsKeyDown(KEY_W);
        kb->down = IsKeyDown(KEY_S);
        kb->left = IsKeyDown(KEY_A);
        kb->right = IsKeyDown(KEY_D);
        kb->shoot = IsKeyPressed(KEY_SPACE);
    }
}

void InputSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    for (auto entity : em.getAllEntities()) {
        auto kb = cm.getComponent<KeyboardControl>(entity);
        auto vel = cm.getComponent<Velocity>(entity);
        if (kb && vel) {
            vel->vx = 0.f;
            vel->vy = 0.f;
            if (kb->up) vel->vy = -200.f;
            if (kb->down) vel->vy = 200.f;
            if (kb->left) vel->vx = -200.f;
            if (kb->right) vel->vx = 200.f;
        }
    }
}

// ShootingSystem
void ShootingSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;

        auto kb = cm.getComponent<KeyboardControl>(e);
        auto pos = cm.getComponent<Position>(e);
        if (kb && kb->shoot && pos) {
            // Create a bullet
            Entity bullet = em.createEntity();
            cm.addComponent(bullet, Position{pos->x + 50, pos->y}); // Adjust position
            cm.addComponent(bullet, Velocity{500.0f, 0.0f}); // Move right
            cm.addComponent(bullet, Bullet{});
            cm.addComponent(bullet, Sprite{cm.getGlobalTexture("bullet"), 10, 10}); // Assign bullet texture
        }
    }
}

// EnemySystem
void EnemySystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    static float spawnTimer = 0.0f;
    spawnTimer += dt;

    if (spawnTimer >= 2.0f) { // Spawn every 2 seconds
        spawnTimer = 0.0f;

        std::cout << "[DEBUG] Spawning enemy..." << std::endl; // Debug print

        Entity enemy = em.createEntity();
        cm.addComponent(enemy, Position{800.0f, static_cast<float>(GetRandomValue(50, 550))});
        cm.addComponent(enemy, Velocity{-100.0f, 0.0f}); // Move left
        cm.addComponent(enemy, Enemy{});
        cm.addComponent(enemy, Sprite{cm.getGlobalTexture("enemy"), 50, 50}); // Assign enemy texture

        std::cout << "[DEBUG] Enemy spawned at X: " << 800.0f << " Y: " << cm.getComponent<Position>(enemy)->y << std::endl;
    }

    // Enemy shooting logic
    for (auto e : em.getAllEntities()) {
        if (!em.isAlive(e)) continue;
        auto enemy = cm.getComponent<Enemy>(e);
        if (enemy) {
            enemy->shootTimer += dt;
            if (enemy->shootTimer >= enemy->shootCooldown) {
                enemy->shootTimer = 0.0f;
                auto pos = cm.getComponent<Position>(e);
                if (pos) {
                    Entity bullet = em.createEntity();
                    cm.addComponent(bullet, Position{pos->x - 20, pos->y});
                    cm.addComponent(bullet, Velocity{-500.0f, 0.0f}); // Move left
                    cm.addComponent(bullet, Bullet{});
                    cm.addComponent(bullet, Sprite{cm.getGlobalTexture("bullet"), 10, 10}); // Assign bullet texture
                }
            }
        }
    }
}

// CollisionSystem
void CollisionSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    static bool playerDead = false;
    static float respawnTimer = 0.0f;

    Entity player = 0; // Assuming player entity ID is 0
    auto playerHealth = cm.getComponent<Health>(player);

    if (playerHealth && playerHealth->current <= 0) {
        em.destroyEntity(player);
        playerDead = true;
        respawnTimer = 3.0f; // Respawn after 3 seconds
    }

    if (playerDead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0) {
            // Respawn player
            player = em.createEntity();
            cm.addComponent(player, Position{100.0f, 300.0f});
            cm.addComponent(player, Velocity{0.0f, 0.0f});
            cm.addComponent(player, KeyboardControl{});
            cm.addComponent(player, Health{3, 3});
            cm.addComponent(player, Sprite{cm.getGlobalTexture("player"), 50, 50});
            playerDead = false;
        }
    }

    // Check collisions between bullets and enemies/player
    for (auto bullet : em.getAllEntities()) {
        if (!em.isAlive(bullet)) continue;
        auto bulletPos = cm.getComponent<Position>(bullet);
        auto bulletComp = cm.getComponent<Bullet>(bullet);
        if (!bulletPos || !bulletComp) continue;

        for (auto entity : em.getAllEntities()) {
            if (!em.isAlive(entity)) continue;
            auto pos = cm.getComponent<Position>(entity);
            auto enemy = cm.getComponent<Enemy>(entity);
            auto health = cm.getComponent<Health>(entity);

            if (pos && (enemy || health)) {
                float dx = bulletPos->x - pos->x;
                float dy = bulletPos->y - pos->y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < 20.0f) { // Collision detected
                    if (enemy) {
                        enemy->health--;
                        if (enemy->health <= 0) {
                            em.destroyEntity(entity); // Destroy enemy
                        }
                    } else if (health) {
                        health->current--;
                        if (health->current <= 0) {
                            em.destroyEntity(entity); // Destroy player
                        }
                    }
                    em.destroyEntity(bullet); // Destroy bullet
                }
            }
        }
    }
}

// AudioSystem
AudioSystem::AudioSystem(Sound sound) : m_sound(sound) {}
AudioSystem::~AudioSystem() {}

void AudioSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    for (auto e : em.getAllEntities()) {
        auto vel = cm.getComponent<Velocity>(e);
        if (vel) {
            float speed = std::sqrt(vel->vx * vel->vx + vel->vy * vel->vy);
            if (speed >= 100.0f && m_sound.frameCount > 0) {
                PlaySound(m_sound);
            }
        }
    }
}