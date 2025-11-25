#pragma once

#include <vector>
#include <cstdint>

namespace lft::reflect {

class PipelineLayoutBuilder {
public:
    PipelineLayoutBuilder(std::vector<uint32_t> code);
};

}
