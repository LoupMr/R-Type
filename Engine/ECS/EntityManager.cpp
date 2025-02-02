#include "EntityManager.hpp"

Entity EntityManager::createEntity() {
    if (!m_freeEntities.empty()) {
        Entity id = *m_freeEntities.begin();
        m_freeEntities.erase(id);
        return id;
    }
    return m_nextEntityId++;
}

void EntityManager::destroyEntity(Entity entity) {
    m_freeEntities.insert(entity);
}

bool EntityManager::isAlive(Entity entity) const {
    return m_freeEntities.find(entity) == m_freeEntities.end();
}

std::vector<Entity> EntityManager::getAllEntities() const {
    std::vector<Entity> entities;
    for (Entity i = 0; i < m_nextEntityId; ++i) {
        if (isAlive(i)) {
            entities.push_back(i);
        }
    }
    return entities;
}
