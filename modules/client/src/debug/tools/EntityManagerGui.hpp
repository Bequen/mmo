#pragma once

#include "debug/ComponentGui.hpp"
#include "entt/entity/fwd.hpp"
#include "world/World.hpp"

#include <entt/entt.hpp>

namespace tw::dbg::tools {

class EntityManagerGui {
private:
    World* m_world;

    entt::entity m_selected;
    std::optional<uint32_t> m_selected_entity;

    void draw_entity_components();

    template<class... Ts>
    void draw_debug_components() {
        (
            [&]() {
                auto character = m_world->registry().try_get<Ts>(m_selected);
                if(character != nullptr) {
                    tw::dbg::ComponentGui<Ts>().draw(character);
                }
            }(),
        ...);
    }

public:
    entt::entity& selected() {
        return m_selected;
    }

    EntityManagerGui(World* world);

    void draw();
};

}
