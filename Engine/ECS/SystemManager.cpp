#include "SystemManager.hpp"
#include <thread>
#include <vector>

namespace Engine {

    void SystemManager::updateAll(float dt, EntityManager& em, ComponentManager& cm) {
        std::vector<std::thread> threads;

        for (auto& sys : m_systems) {
            threads.emplace_back([&, sys] {
                sys->update(dt, em, cm);
            });
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

}
