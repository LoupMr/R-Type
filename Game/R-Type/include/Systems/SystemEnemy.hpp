#ifndef ENEMY_SYSTEM_HPP
#define ENEMY_SYSTEM_HPP

#include "ECS.hpp"
#include "Components/EnemyComponent.hpp"

class EnemySystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto enemy = cm.getComponent<Enemy>(e);
            if (enemy && enemy->isDestroyed) {
                em.destroyEntity(e);
            }
        }
    }
};

#endif // ENEMY_SYSTEM_HPP