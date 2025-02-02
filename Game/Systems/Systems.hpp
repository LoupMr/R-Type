#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "ECS/System.hpp"
#include "Components/Components.hpp"

class RenderSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class MovementSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class InputSystem : public System {
public:
    void handleInput(EntityManager& em, ComponentManager& cm);
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class ShootingSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class EnemySystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class CollisionSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;
};

class AudioSystem : public System {
public:
    AudioSystem(Sound sound);
    ~AudioSystem();
    void update(float dt, EntityManager& em, ComponentManager& cm) override;

private:
    Sound m_sound;
};

#endif // SYSTEMS_HPP