#ifndef COMPONENTMANAGER_HPP
#define COMPONENTMANAGER_HPP

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <type_traits>

#include "../EntityManager.hpp"
#include "ComponentArray.hpp"

class ComponentManager {
public:
    template<typename T>
    void addComponent(Entity entity, const T& component) {
        auto typeIdx = std::type_index(typeid(T));
        if (m_componentArrays.find(typeIdx) == m_componentArrays.end()) {
            m_componentArrays[typeIdx] = std::make_shared<ComponentArray<T>>();
        }
        auto compArray = std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeIdx]);
        compArray->addComponent(entity, component);
    }
    template<typename T>
    void removeComponent(Entity entity) {
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_componentArrays.find(typeIdx);
        if (it != m_componentArrays.end()) {
            auto compArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
            compArray->removeComponent(entity);
        }
    }
    template<typename T>
    bool hasComponent(Entity entity) {
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_componentArrays.find(typeIdx);
        if (it != m_componentArrays.end()) {
            auto compArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
            return compArray->hasComponent(entity);
        }
        return false;
    }
    template<typename T>
    T* getComponent(Entity entity) {
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_componentArrays.find(typeIdx);
        if (it != m_componentArrays.end()) {
            auto compArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
            return compArray->getComponent(entity);
        }
        return nullptr;
    }
private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_componentArrays;
};

#endif // COMPONENTMANAGER_HPP