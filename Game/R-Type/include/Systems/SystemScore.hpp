#ifndef SCORE_SYSTEM_HPP
#define SCORE_SYSTEM_HPP

#include "ECS.hpp"
#include "Components/ScoreComponent.hpp"
#include "Components/EnemyComponent.hpp"

class ScoreSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto score = cm.getComponent<Score>(e);
            auto enemy = cm.getComponent<Enemy>(e);
            if (score && enemy && enemy->isDestroyed) {
                score->value += 100;
                em.destroyEntity(e);
            }
        }
    }
};

#endif // SCORE_SYSTEM_HPP