#ifndef SYSTEM_HPP
#define SYSTEM_HPP

// -----------------------
// System Base + Manager
// -----------------------
class EntityManager;      // Forward-declaration
class ComponentManager;   // Forward-declaration

class System {
public:
    virtual ~System() = default;
    virtual void update(float dt, EntityManager& em, ComponentManager& cm) = 0;
};

#endif // SYSTEM_HPP