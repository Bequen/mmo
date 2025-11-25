#pragma once

#include <string>

namespace tw::dbg {

class DebugComponent {
private:
    std::string m_name;

public:
    inline const std::string& name() const {
        return m_name;
    }

    DebugComponent(const std::string& name) :
        m_name{name} {
        }
};

}
