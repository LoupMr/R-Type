#ifndef ENGINE_COLLISIONSYSTEM_HPP
#define ENGINE_COLLISIONSYSTEM_HPP

#include <iostream>
#include "System.hpp"

namespace Engine {

    struct Collider {
        float x, y;
        float width, height;
    };

    class EngineCollisionSystem : public System {
    public:
        void update(float dt, EntityManager& em, ComponentManager& cm) override;

    private:
        bool checkOverlap(const Collider& a, const Collider& b);
    };

}

#endif // ENGINE_COLLISIONSYSTEM_HPP
