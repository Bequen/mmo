#include "ShaderManager.hpp"

namespace tw::io {

ShaderManager::ShaderManager(const Gpu* gpu, const Files* files) :
    m_files(files),
    m_shader_builder(gpu)
{
}

Shader ShaderManager::load_shader(const std::string& name) {
    return m_shader_builder.from_binary(m_files->read_file_binary(path("shaders") / name));
}

}
