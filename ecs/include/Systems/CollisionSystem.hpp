#ifndef COLLISIONSYSTEM_HPP
#define COLLISIONSYSTEM_HPP

#include <cmath>
#include <iostream>

#include "System.hpp"
#include "../EntityManager.hpp"
#include "../Components/ComponentManager.hpp"

// --------------------------------
// Collision Components & System
// --------------------------------

// A simple bounding-box collider
struct Collider {
    float x, y;       // offset relative to the entity's position
    float width, height;
};

// Example collision system that checks for overlapping AABBs
class CollisionSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        (void)dt; // Not needed for purely geometric checks

        const auto& entities = em.getAllEntities();
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity e1 = entities[i];
            if (!em.isAlive(e1)) continue;
            auto c1 = cm.getComponent<Collider>(e1);
            if (!c1) continue;

            // Optional: also get Position if you want to offset collider by entity's position
            // auto pos1 = cm.getComponent<Position>(e1);

            for (size_t j = i + 1; j < entities.size(); ++j) {
                Entity e2 = entities[j];
                if (!em.isAlive(e2)) continue;
                auto c2 = cm.getComponent<Collider>(e2);
                if (!c2) continue;

                // Check bounding box overlap
                if (checkOverlap(*c1, *c2)) {
                    // You could handle the collision event:
                    // - Print a message
                    // - Apply damage
                    // - Bounce velocities
                    // - Or store "collision events" in a queue
                    // For simplicity, we just print:
                    std::cout << "Collision detected between E" << e1 
                              << " and E" << e2 << "\n";
                }
            }
        }
    }

private:
    bool checkOverlap(const Collider& a, const Collider& b) {
        // Simple AABB overlap check
        bool overlapX = (a.x < (b.x + b.width)) && ((a.x + a.width) > b.x);
        bool overlapY = (a.y < (b.y + b.height)) && ((a.y + a.height) > b.y);
        return overlapX && overlapY;
    }
};

#endif // COLLISIONSYSTEM_HPP