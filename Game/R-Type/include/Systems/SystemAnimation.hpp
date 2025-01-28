#ifndef ANIMATION_SYSTEM_HPP
#define ANIMATION_SYSTEM_HPP

#include "ECS.hpp"
#include "Components/AnimationComponent.hpp"

class AnimationSystem : public System {
public:
    void update(float dt, EntityManager& em, ComponentManager& cm) override {
        for (auto e : em.getAllEntities()) {
            if (!em.isAlive(e)) continue;
            auto anim = cm.getComponent<Animation>(e);
            if (anim) {
                anim->elapsedTime += dt;
                if (anim->elapsedTime >= anim->frameDuration) {
                    anim->elapsedTime = 0;
                    anim->frame = (anim->frame + 1) % anim->frameCount;
                }
            }
        }
    }
};

#endif // ANIMATION_SYSTEM_HPP