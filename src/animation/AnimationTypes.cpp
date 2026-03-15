#include "AnimationTypes.hpp"

AnimationTypes makeAnimationTypes(std::initializer_list<AnimationType> setList) {
    AnimationTypes types;
    for (auto t : setList) {
        if (t == ANIMATION_TYPE_NIL) continue;
        types.setIndexOn((size_t)t);
    }
    return types;
}
