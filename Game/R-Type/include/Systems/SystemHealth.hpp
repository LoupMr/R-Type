#ifndef HEALTH_SYSTEM_HPP
#define HEALTH_SYSTEM_HPP

#include "ECS.hpp"
#include "Components/HealthComponent.hpp"

class HealthSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto health = cm.getComponent<Health>(e);
            if (health && health->current <= 0) {
                em.destroyEntity(e);
            }
        }
    }
};

#endif // HEALTH_SYSTEM_HPP