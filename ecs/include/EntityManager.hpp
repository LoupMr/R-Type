#ifndef ENTITYMANAGER_HPP
#define ENTITYMANAGER_HPP

#include <cstdint>
#include <vector>
#include <algorithm>

// ---------------------------
// Basic ECS Infrastructure
// ---------------------------
using Entity = std::uint32_t;

class EntityManager {
public:
    Entity createEntity() {
        Entity e = m_nextEntity++;
        m_entities.push_back(e);
        return e;
    }
    void destroyEntity(Entity entity) {
        m_destroyed.push_back(entity);
    }
    const std::vector<Entity>& getAllEntities() const {
        return m_entities;
    }
    void processDestroyed() {
        for (auto e : m_destroyed) {
            m_entities.erase(std::remove(m_entities.begin(), m_entities.end(), e), m_entities.end());
        }
        m_destroyed.clear();
    }
    bool isAlive(Entity entity) const {
        return (std::find(m_destroyed.begin(), m_destroyed.end(), entity) == m_destroyed.end()) &&
               (std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end());
    }
private:
    std::vector<Entity> m_entities;
    std::vector<Entity> m_destroyed;
    Entity m_nextEntity = 0;
};

#endif // ENTITYMANAGER_HPP