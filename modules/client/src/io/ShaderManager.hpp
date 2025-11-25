#pragma once

#include <shaders/Shader.hpp>
#include <shaders/SpirvShaderBuilder.hpp>

#include "io/Files.hpp"

namespace tw::io {

/**
 * Unifies and simplifies shader loading
 */
class ShaderManager {
    const Files *m_files;
    const SpirvShaderBuilder m_shader_builder;

public:
    ShaderManager(const Gpu* gpu, const Files *files);

    Shader load_shader(const std::string& name);
};

}
