#pragma once

namespace tw::dbg {

template<typename T>
class ComponentGui {
public:
    void draw(T* instance);
};

}
