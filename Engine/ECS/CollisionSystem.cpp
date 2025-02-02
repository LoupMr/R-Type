#include "CollisionSystem.hpp"

void CollisionSystem::update(float dt, EntityManager& em, ComponentManager& cm) {
    (void)dt;

    const auto& entities = em.getAllEntities();
    for (size_t i = 0; i < entities.size(); ++i) {
        Entity e1 = entities[i];
        if (!em.isAlive(e1)) continue;
        auto c1 = cm.getComponent<Collider>(e1);
        if (!c1) continue;

        for (size_t j = i + 1; j < entities.size(); ++j) {
            Entity e2 = entities[j];
            if (!em.isAlive(e2)) continue;
            auto c2 = cm.getComponent<Collider>(e2);
            if (!c2) continue;

            // Check bounding box overlap
            if (checkOverlap(*c1, *c2)) {
                std::cout << "Collision detected between E" << e1
                          << " and E" << e2 << "\n";
            }
        }
    }
}

bool CollisionSystem::checkOverlap(const Collider& a, const Collider& b) {
    bool overlapX = (a.x < (b.x + b.width)) && ((a.x + a.width) > b.x);
    bool overlapY = (a.y < (b.y + b.height)) && ((a.y + a.height) > b.y);
    return overlapX && overlapY;
}
