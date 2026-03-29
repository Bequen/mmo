#pragma once

#include <volk.h>

class ImageResource {
public:
	VkImage image;
	VkImageView image_view;
    VkExtent2D extent;

	ImageResource(const ImageResource&) = default;
	ImageResource(ImageResource&) = default;
	ImageResource(ImageResource&&) = default;

	ImageResource(VkImage image, VkImageView image_view, VkExtent2D extent) :
		image(image),
		image_view(image_view),
        extent(extent) {
	};
};

class BufferResource {
public:
	VkBuffer buffer;
	VkDeviceSize size;

	BufferResource(VkBuffer buffer, VkDeviceSize size) :
		buffer(buffer),
		size(size) {
	};
};
