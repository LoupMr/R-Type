#ifndef SYSTEMMANAGER_HPP
#define SYSTEMMANAGER_HPP

#include <vector>
#include <memory>
#include <type_traits>
#include "System.hpp"
#include "../EntityManager.hpp"
#include "../Components/ComponentManager.hpp"

// (Comment block for System Manager is in the same region as System,
// but here we separate them for clarity.)

class SystemManager {
public:
    template<typename T, typename... Args>
    std::shared_ptr<T> addSystem(Args&&... args) {
        static_assert(std::is_base_of<System, T>::value, "T must inherit from System");
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        m_systems.push_back(system);
        return system;
    }

    void updateAll(float dt, EntityManager& em, ComponentManager& cm) {
        for (auto& sys : m_systems) {
            sys->update(dt, em, cm);
        }
    }
private:
    std::vector<std::shared_ptr<System>> m_systems;
};

#endif // SYSTEMMANAGER_HPP