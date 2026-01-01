#pragma once

#include <string>

#include "AnimationEnum.hpp"
#include "Array.hpp"
#include "Pointer.hpp"

struct KeyframeFlyweight {
    const char* name;

    static const Array<KeyframeFlyweight, ANIMATION_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
