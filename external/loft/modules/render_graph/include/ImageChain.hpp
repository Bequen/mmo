#pragma once

#include <vector>

#include "props.hpp"
#include "resources/ImageView.hpp"
#include "Swapchain.hpp"

/**
 * Chain of image views. Used for swapchain and render graph output.
 */
struct ImageChain {
private:
    std::vector<ImageView> m_images;
	VkFormat m_format;
	VkExtent2D m_extent;

	VkImageLayout m_layout;

public:
	GET(m_layout, layout);
	GET(m_format, format);
	GET(m_extent, extent);

    [[nodiscard]] inline uint32_t count() const {
        return m_images.size();
    }

    [[nodiscard]] inline const std::vector<ImageView>& views() const {
        return m_images;
    }

    ImageChain(const ImageChain& other) :
        m_format(other.m_format),
        m_extent(other.m_extent), 
        m_layout(other.m_layout) {

        m_images.clear();
        std::copy(other.m_images.begin(), other.m_images.end(),
                std::back_inserter(m_images));
    }

    ImageChain& operator=(const ImageChain& other) {
        m_images.clear();
        std::copy(other.m_images.begin(), other.m_images.end(),
                std::back_inserter(m_images));
        m_format = other.m_format;
        m_extent = other.m_extent;
        m_layout = other.m_layout;
        return *this;
    }

    ImageChain(ImageChain&&) = delete;
    ImageChain& operator=(ImageChain&&) = delete;

	ImageChain(VkFormat format,
			VkExtent2D extent,
			VkImageLayout layout,
			const std::vector<ImageView>& images) :
		m_format(format),
		m_extent(extent),
		m_layout(layout),
		m_images(images) {

    }

	static ImageChain from_swapchain(const Swapchain& swapchain) {
		return ImageChain(swapchain.format().format,
				swapchain.extent(),
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				swapchain.views());
	}
};

