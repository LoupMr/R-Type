#include "../include/EntityManager.hpp"
#include "../include/Components/ComponentManager.hpp"
#include "../include/Systems/SystemManager.hpp"
#include "../include/Systems/CollisionSystem.hpp"
#include <iostream>
#include <cassert>

int main() {
    EntityManager em;
    ComponentManager cm;
    SystemManager sm;

    auto collisionSystem = sm.addSystem<CollisionSystem>();

    // Create some entities
    Entity e1 = em.createEntity();
    Entity e2 = em.createEntity();
    std::cout << "Created entities: " << e1 << ", " << e2 << std::endl;

    // Add collider components
    cm.addComponent<Collider>(e1, { 0.f, 0.f, 100.f, 100.f });
    cm.addComponent<Collider>(e2, { 50.f, 50.f,  80.f,  80.f });
    std::cout << "Added collider components to entities: " << e1 << ", " << e2 << std::endl;

    // Check if entities are alive
    assert(em.isAlive(e1));
    assert(em.isAlive(e2));
    std::cout << "Entities are alive: " << e1 << ", " << e2 << std::endl;

    // Update to detect collisions
    sm.updateAll(0.016f, em, cm);
    std::cout << "Updated systems to detect collisions." << std::endl;

    // Destroy an entity and process destruction
    em.destroyEntity(e1);
    std::cout << "Destroyed entity: " << e1 << std::endl;
    em.processDestroyed();
    std::cout << "Processed destroyed entities." << std::endl;

    // Check if entity is destroyed
    assert(!em.isAlive(e1));
    assert(em.isAlive(e2));
    std::cout << "Entity " << e1 << " is destroyed. Entity " << e2 << " is still alive." << std::endl;

    return 0;
}