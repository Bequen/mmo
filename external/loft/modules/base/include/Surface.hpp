
#pragma once

#include <utility>
#include <volk.h>

#include "Instance.hpp"

/**
 * Surface is a target for the Gpu instance to render to.
 */
struct Surface {
private:
    VkInstance m_instance;
    VkSurfaceKHR m_vk_surface;

public:
    GET(m_vk_surface, surface);

    Surface(const Instance* instance, VkSurfaceKHR vk_surface) :
        m_instance(instance->instance()), m_vk_surface(vk_surface) {}

    Surface(const Surface&) = delete;

    Surface(Surface&& other) noexcept :
        m_instance(std::exchange(other.m_instance, nullptr)),
        m_vk_surface(std::exchange(other.m_vk_surface, nullptr)) {
        
    }

    Surface& operator=(const Surface& other) = delete;
    Surface& operator=(const Surface&& other) noexcept {
        this->m_vk_surface = other.m_vk_surface;
        this->m_instance = other.m_instance;
        return *this;
    }

    ~Surface() {
        vkDestroySurfaceKHR(m_instance, m_vk_surface, nullptr);
    }
};
