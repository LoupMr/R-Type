#ifndef ENGINE_SYSTEM_HPP
#define ENGINE_SYSTEM_HPP

#include "EntityManager.hpp"
#include "ComponentManager.hpp"

namespace Engine {

    class System {
    public:
        virtual ~System() = default;
        virtual void update(float dt, EntityManager& em, ComponentManager& cm) = 0;
    };

}

#endif // ENGINE_SYSTEM_HPP
