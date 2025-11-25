#include "PipelineLayoutBuilder.hpp"

#include "shaders/PipelineLayoutBuilder.hpp"

#include <print>

#include "spirv_reflect.h"

namespace lft::reflect {

std::string SpvReflectResultToString(SpvReflectResult result) {
    switch (result) {
        case SPV_REFLECT_RESULT_SUCCESS: return "Success";
        case SPV_REFLECT_RESULT_NOT_READY: return "Not ready";
        case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED: return "Parse failed";
        case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED: return "Allocation failed";
        case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED: return "Range exceeded";
        case SPV_REFLECT_RESULT_ERROR_NULL_POINTER: return "Null pointer";
        case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR: return "Internal error";
        case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH: return "Count mismatch";
        case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND: return "Element not found";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE: return "Invalid SPIR-V code size";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER: return "Invalid SPIR-V magic number";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF: return "Unexpected end of SPIR-V code";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE: return "Invalid SPIR-V ID reference";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW: return "SPIR-V set number overflow";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS: return "Invalid SPIR-V storage class";
        default: return "Unknown error";
    }
}

PipelineLayoutBuilder::PipelineLayoutBuilder(std::vector<uint32_t> code) {
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(
            code.size() * 4, (const char*)code.data(), &module
    );

    if(result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("{}", SpvReflectResultToString(result));
        return;
    }

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());

    for(auto& set : sets) {
        std::println("Set: {}", set->set);

        for(uint32_t i = 0; i < set->binding_count; i++) {
            std::println("Binding: {}", set->bindings[i]->binding);
            
            VkDescriptorSetLayoutBinding binding = {
                .binding = set->bindings[i]->binding,
                .descriptorType = (VkDescriptorType)set->bindings[i]->descriptor_type,
                .descriptorCount = set->bindings[i]->count,
            };
        }
    }
}

}
