#ifndef COMPONENTARRAY_HPP
#define COMPONENTARRAY_HPP

#include <cstdint>
#include <vector>
#include "EntityManager.hpp"

template<typename T>
class ComponentArray {
public:
    void addComponent(Entity entity, const T& component) {
        if (entity >= m_data.size()) {
            m_data.resize(entity + 1);
            m_hasData.resize(entity + 1, false);
        }
        m_data[entity] = component;
        m_hasData[entity] = true;
    }

    void removeComponent(Entity entity) {
        if (entity < m_data.size()) {
            m_hasData[entity] = false;
        }
    }

    bool hasComponent(Entity entity) const {
        return (entity < m_data.size()) && m_hasData[entity];
    }

    T* getComponent(Entity entity) {
        if (hasComponent(entity)) {
            return &m_data[entity];
        }
        return nullptr;
    }

private:
    std::vector<T> m_data;
    std::vector<bool> m_hasData;
};

#endif // COMPONENTARRAY_HPP