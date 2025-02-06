#include "SystemManager.hpp"

namespace Engine {
    void SystemManager::updateAll(float dt, EntityManager& em, ComponentManager& cm) {
        for (auto& sys : m_systems) {
            sys->update(dt, em, cm);
        }
    }
}
