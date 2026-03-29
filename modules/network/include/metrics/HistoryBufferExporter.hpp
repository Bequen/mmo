#pragma once

#include "metrics/HistoryBuffer.hpp"

#include <filesystem>
#include <fstream>



/**
 * Exports HistoryBuffer into a CSV.
 */
template<typename TKey, typename TValue>
class HistoryBufferExporter {
    std::fstream m_output_file;

public:
    HistoryBufferExporter(std::filesystem::path output_path) {
        m_output_file = std::fstream(output_path, std::ios::out);
        if(!m_output_file.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }
    }

    /**
     * Writes the history buffer if full and clears it.
     * @return bool: true if buffer was full and exported. False otherwise.
     */
    bool write(const uint32_t entity, TKey key, TValue value) {
        m_output_file << entity << "," << key << "," << value.x << "," << value.y << "," << value.z << "\n";
        return true;
    }
};
