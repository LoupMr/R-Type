#ifndef SYSTEMMANAGER_HPP
#define SYSTEMMANAGER_HPP

#include <vector>
#include <memory>
#include <type_traits>
#include "System.hpp"
#include "EntityManager.hpp"
#include "ComponentManager.hpp"


class SystemManager {
public:
    template<typename T, typename... Args>
    std::shared_ptr<T> addSystem(Args&&... args);

    void updateAll(float dt, EntityManager& em, ComponentManager& cm);

private:
    std::vector<std::shared_ptr<System>> m_systems;
};

template<typename T, typename... Args>
std::shared_ptr<T> SystemManager::addSystem(Args&&... args) {
    static_assert(std::is_base_of<System, T>::value, "T must inherit from System");
    auto system = std::make_shared<T>(std::forward<Args>(args)...);
    m_systems.push_back(system);
    return system;
}

#endif // SYSTEMMANAGER_HPP
