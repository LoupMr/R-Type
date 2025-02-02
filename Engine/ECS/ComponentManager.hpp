#ifndef COMPONENTMANAGER_HPP
#define COMPONENTMANAGER_HPP

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <type_traits>
#include <raylib.h> // Include Raylib for Texture2D

#include "EntityManager.hpp"
#include "ComponentArray.hpp"

class ComponentManager {
public:
    // Add a component to an entity
    template<typename T>
    void addComponent(Entity entity, const T& component) {
        auto typeIdx = std::type_index(typeid(T));
        if (m_componentArrays.find(typeIdx) == m_componentArrays.end()) {
            m_componentArrays[typeIdx] = std::make_shared<ComponentArray<T>>();
        }
        auto compArray = std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeIdx]);
        compArray->addComponent(entity, component);
    }

    // Remove a component from an entity
    template<typename T>
    void removeComponent(Entity entity) {
        auto typeIdx = std::type_index(typeid(T));
        auto it = m_componentArrays.find(typeIdx);
        if (it != m_componentArrays.end()) {
            auto compArray = std::static_pointer_cast<ComponentArray<T>>(it->second);
            compArray->removeComponent(entity);
        }
    }

    // Check if an entity has a component
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

    // Get a component from an entity
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

    // Store a global texture
    void setGlobalTexture(const std::string& name, Texture2D texture) {
        m_globalTextures[name] = texture;
    }

    // Get a global texture
    Texture2D getGlobalTexture(const std::string& name) {
        auto it = m_globalTextures.find(name);
        if (it != m_globalTextures.end()) {
            return it->second;
        }
        return Texture2D{}; // Return an empty texture if not found
    }

private:
    // Component arrays
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_componentArrays;

    // Global textures
    std::unordered_map<std::string, Texture2D> m_globalTextures;
};

#endif // COMPONENTMANAGER_HPP