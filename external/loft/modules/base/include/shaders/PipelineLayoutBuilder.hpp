#pragma once

#include <vector>
#include <stdexcept>

#include <volk.h>

#include "Gpu.hpp"

namespace lft {
class PipelineLayoutBuilder {
private:
	std::vector<VkDescriptorSetLayout> m_layouts;
	std::vector<VkPushConstantRange> m_pushConstantRanges;

	uint32_t m_numLayouts;

public:
	PipelineLayoutBuilder() :
	m_layouts(4), m_pushConstantRanges(), m_numLayouts(0) {
	}

	PipelineLayoutBuilder& input_set(uint32_t idx, VkDescriptorSetLayout setLayout) {
		if(idx >= m_layouts.size()) {
			throw std::runtime_error("As of now, only 4 descriptor set layouts are supported");
		}

		m_layouts[idx] = setLayout;
		m_numLayouts = std::max(m_numLayouts, idx + 1);
		return *this;
	}

    PipelineLayoutBuilder& push_constant_range(uint32_t offset, uint32_t size, VkShaderStageFlags stages) {
        m_pushConstantRanges.push_back({
            .stageFlags = stages,
            .offset = offset,
            .size = size,
        });
        return *this;
    }

	VkPipelineLayout build(const Gpu* gpu) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = m_numLayouts,
                .pSetLayouts = m_layouts.data(),
                .pushConstantRangeCount = (uint32_t)m_pushConstantRanges.size(),
                .pPushConstantRanges = m_pushConstantRanges.data(),
        };

        VkPipelineLayout inputLayout = VK_NULL_HANDLE;
        if (vkCreatePipelineLayout(gpu->dev(), &pipelineLayoutInfo, nullptr, &inputLayout)) {
            throw std::runtime_error("Failed to build pipeline layout");
        }

        return inputLayout;
    }
};
}
