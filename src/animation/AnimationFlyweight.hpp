#pragma once

#include <string>

#include "AnimationSemantic.hpp"
#include "AnimationEnum.hpp"
#include "AnimationTypes.hpp"
#include "Array.hpp"

struct AnimationFlyweight {
    const char* name;
    int index;
    AnimationTypes types;
    AnimationTypes semantics;

    long baseDuration;

    Array<AnimationSemantic, KEYFRAME_DATA_ARRAY_SIZE> data;

    static const Array<AnimationFlyweight, ANIMATION_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, AnimationEnum& output);

    static Maybe<AnimationEnum> queryAnimation(const AnimationEnum& animation, const AnimationTypes& semantics);
};
