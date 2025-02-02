#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "EntityManager.hpp"
#include "ComponentManager.hpp"

class System {
public:
    virtual ~System() = default;
    virtual void update(float dt, EntityManager& em, ComponentManager& cm) = 0;
};

#endif // SYSTEM_HPP