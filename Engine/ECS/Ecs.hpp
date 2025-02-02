#ifndef ECS_HPP
#define ECS_HPP

#include "EntityManager.hpp"
#include "ComponentArray.hpp"
#include "ComponentManager.hpp"
#include "System.hpp"
#include "SystemManager.hpp"
#include "CollisionSystem.hpp"

class ECS {
public:
    ECS() : entityManager(), componentManager(), systemManager() {}

    Entity createEntity() {
        return entityManager.createEntity();
    }

    void destroyEntity(Entity entity) {
        entityManager.destroyEntity(entity);
    }

    bool isAlive(Entity entity) const {
        return entityManager.isAlive(entity);
    }

    template<typename T>
    void addComponent(Entity entity, const T& component) {
        componentManager.addComponent<T>(entity, component);
    }

    template<typename T>
    void removeComponent(Entity entity) {
        componentManager.removeComponent<T>(entity);
    }

    template<typename T>
    bool hasComponent(Entity entity) {
        return componentManager.hasComponent<T>(entity);
    }

    template<typename T>
    T* getComponent(Entity entity) {
        return componentManager.getComponent<T>(entity);
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> addSystem(Args&&... args) {
        return systemManager.addSystem<T>(std::forward<Args>(args)...);
    }

    void update(float dt) {
        systemManager.updateAll(dt, entityManager, componentManager);
    }

private:
    EntityManager entityManager;
    ComponentManager componentManager;
    SystemManager systemManager;
};

#endif // ECS_HPP