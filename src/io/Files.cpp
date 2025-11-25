#include "Files.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <print>

#if __linux__
#include <linux/limits.h>
#include <unistd.h>
#include <err.h>
#elif _WIN32
#include <windows.h>
#else
#error "Unsupported platform"
#endif

namespace tw::io {

#ifdef __linux__
const char DIRECTORY_SEPARATOR = '/';
#elif _WIN32
const char DIRECTORY_SEPARATOR = '\\';
#else
#error "Unsupported platform"
#endif

#include "IoException.hpp"

Files::Files(char** argv, int32_t argc) {
	char *pBuffer = nullptr;
#ifdef __linux__
	pBuffer = (char*)malloc(PATH_MAX);
	size_t bytes = readlink("/proc/self/exe", pBuffer, PATH_MAX);
    pBuffer[bytes] = '\0';
#elif _WIN32
	pBuffer = (char*)malloc(_MAX_PATH);
    int bytes = GetModuleFileName(NULL, pBuffer, _MAX_PATH);
    if(bytes >= 0)
        pBuffer[bytes] = '\0';
#else
#error "Unsupported platform"
#endif
	unsigned int lastSlash = 0;
	for(size_t i = bytes; i > 0; i--) {
		if(pBuffer[i] == DIRECTORY_SEPARATOR) {
			lastSlash = i + 1;
			break;
		}
	}

	pBuffer[lastSlash] = '\0';

    m_exe_path = std::filesystem::path(std::string(pBuffer));
}

std::filesystem::path Files::get_path_of(const path& name) const {
    return m_exe_path / name;
}

std::vector<uint32_t> Files::read_file_binary(const path& name) const {
    auto path = get_path_of(name);

    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if(stream.fail()) {
        throw tw::io::IoException(path, std::strerror(errno));
    }

    size_t length = stream.tellg();
    std::vector<uint32_t> content(length);

    stream.seekg(0);
    stream.read((char*)content.data(), length);

    stream.close();

    return content;
}

}
