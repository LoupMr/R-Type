#ifndef GAME_SYSTEMS_HPP
#define GAME_SYSTEMS_HPP

#include "Engine/ECS/System.hpp"
#include "Engine/ECS/EntityManager.hpp"
#include "Engine/ECS/ComponentManager.hpp"
#include "Game/Components/Components.hpp"
#include <raylib.h>

class RenderSystem : public Engine::System {
public:
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;
};

class MovementSystem : public Engine::System {
public:
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;
};

class InputSystem : public Engine::System {
public:
    void handleInput(Engine::EntityManager& em, Engine::ComponentManager& cm);
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;
};

class ShootingSystem : public Engine::System {
public:
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;
};

class EnemySystem : public Engine::System {
public:
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;
};

class CollisionSystem : public Engine::System {
public:
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;

private:
    bool playerDead = false;
    float respawnTimer = 0.0f;
};

class AudioSystem : public Engine::System {
public:
    AudioSystem(Sound sound);
    ~AudioSystem();
    void update(float dt, Engine::EntityManager& em, Engine::ComponentManager& cm) override;

private:
    Sound m_sound;
};

#endif // GAME_SYSTEMS_HPP