#pragma once

struct EntityId {
};

class EntityManager {
public:
    EntityId add_entity();

    void add_component(EntityId entity);
};
