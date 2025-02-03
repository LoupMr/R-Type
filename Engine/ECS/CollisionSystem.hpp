#ifndef COLLISIONSYSTEM_HPP
#define COLLISIONSYSTEM_HPP

#include <iostream>
#include "System.hpp"
#include "EntityManager.hpp"
#include "ComponentManager.hpp"

struct Collider {
    float x, y;
    float width, height;
};

class CollisionSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override;

private:
    bool checkOverlap(const Collider& a, const Collider& b);
};

#endif // COLLISIONSYSTEM_HPP
