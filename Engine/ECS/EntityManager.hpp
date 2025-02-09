#ifndef ENGINE_ENTITYMANAGER_HPP
#define ENGINE_ENTITYMANAGER_HPP

#include <cstdint>
#include <vector>
#include <unordered_set>

namespace Engine {
    using Entity = std::uint32_t;

    class EntityManager {
    public:
        Entity createEntity();
        void destroyEntity(Entity entity);
        bool isAlive(Entity entity) const;
        std::vector<Entity> getAllEntities() const;

    private:
        Entity m_nextEntityId{0};
        std::unordered_set<Entity> m_freeEntities;
    };
}

#endif // ENGINE_ENTITYMANAGER_HPP
