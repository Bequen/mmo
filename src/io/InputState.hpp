#pragma once

#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <imgui_impl_sdl2.h>

#include <Window.hpp>

#include "common.hpp"
#include "debug/ComponentGui.hpp"
#include "imgui.h"

namespace tw::io {

class InputManager {
private:
    lft::win::Window* m_window;

    bool m_is_quit;

    float m_velocity_x;
    float m_velocity_y;

    bool m_is_jumping;

    bool m_is_pressed[4];

    float m_motion_x;
    float m_motion_y;

public:
    GET(m_is_quit, is_quit);

    float axis_y() const {
        float result = 0.0f;
        if(m_is_pressed[0]) {
            result += 1.0f;
        } if(m_is_pressed[1]) {
            result -= 1.0f;
        }

        return result;
    }

    float axis_x() const {
        float result = 0.0f;
        if(m_is_pressed[2]) {
            result += 1.0f;
        } if(m_is_pressed[3]) {
            result -= 1.0f;
        }

        return result;
    }

    GET(m_motion_x, motion_x);
    GET(m_motion_y, motion_y);

    InputManager(lft::win::Window* window) :
        m_window(window),
        m_is_quit(false)
    {
        m_is_pressed[0] = m_is_pressed[1] = m_is_pressed[2] = m_is_pressed[3] = false;
    }

    void update() {
        SDL_Event event;
        m_motion_x = 0.0f;
        m_motion_y = 0.0f;

        while(m_window->poll_event(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch(event.type) {
                case SDL_QUIT:
                    m_is_quit = true;
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    switch(event.key.keysym.sym) {
                        case SDLK_w:
                            m_is_pressed[0] = event.key.state;
                            break;
                        case SDLK_s:
                            m_is_pressed[1] = event.key.state;
                            break;
                        case SDLK_a:
                            m_is_pressed[2] = event.key.state;
                            break;
                        case SDLK_d:
                            m_is_pressed[3] = event.key.state;
                            break;
                    }
                } break;
                case SDL_MOUSEMOTION:
                    m_motion_x = event.motion.xrel;
                    m_motion_y = event.motion.yrel;
                    break;
            }
        }
    }
};


class InputState {
public:

};

}

namespace tw::dbg {

template<>
class ComponentGui<tw::io::InputManager> {
public:
    void draw(tw::io::InputManager* instance) {
        ImGui::SeparatorText("Character Body Component");

        float motion[2] = {instance->motion_x(), instance->motion_y()};
        ImGui::InputFloat2("Motion", motion);

        float axis[2] = {instance->axis_x(), instance->axis_y()};
        ImGui::InputFloat2("Axis", axis);
    }
};

}
