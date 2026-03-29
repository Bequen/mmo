#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>


namespace tw::io {

using namespace std::filesystem;

/**
 * Manages access to a application specific directory for storing assets.
 */
class Files {
    path m_exe_path;

public:
    Files(char** argv, int32_t argc);

    path get_path_of(const path& path) const;

    std::vector<uint32_t> read_file_binary(const path& path) const;
};

}
