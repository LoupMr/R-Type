#ifndef COMPONENTARRAY_HPP
#define COMPONENTARRAY_HPP

#include <cstdint>
#include <vector>

// ---------------------------------------------------
// Component Storage (template-based)
// ---------------------------------------------------
template<typename T>
class ComponentArray {
public:
    void addComponent(std::uint32_t entity, const T& component) {
        if (entity >= m_data.size()) {
            m_data.resize(entity + 1);
            m_hasData.resize(entity + 1, false);
        }
        m_data[entity] = component;
        m_hasData[entity] = true;
    }
    void removeComponent(std::uint32_t entity) {
        if (entity < m_data.size()) {
            m_hasData[entity] = false;
        }
    }
    bool hasComponent(std::uint32_t entity) const {
        return (entity < m_data.size()) && m_hasData[entity];
    }
    T* getComponent(std::uint32_t entity) {
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