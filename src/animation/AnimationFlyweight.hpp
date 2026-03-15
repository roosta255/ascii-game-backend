#pragma once

#include <string>

#include "AnimationEnum.hpp"
#include "AnimationTypes.hpp"
#include "Array.hpp"

struct AnimationFlyweight {
    const char* name;
    AnimationTypes types;
    const char* audio;
    const char* spritesheet;

    static const Array<AnimationFlyweight, ANIMATION_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, AnimationEnum& output);
};
