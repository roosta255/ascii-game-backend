#include "AnimationTypes.hpp"

AnimationTypes makeAnimationTypes(std::initializer_list<AnimationType> setList) {
    AnimationTypes types;
    for (auto t : setList) {
        if (t == ANIMATION_TYPE_NIL) continue;
        types.setIndexOn((size_t)t);
    }
    return types;
}

int get_animation_semantics(const AnimationTypes& types, AnimationSemantic out[2]) {
    int count = 0;
    for (int i = 0; i < ANIMATION_SEMANTIC_COUNT && count < 2; ++i) {
        if (types[(size_t)(ANIMATION_TYPE_COUNT + i)].orElse(false)) {
            out[count++] = AnimationSemantic(i);
        }
    }
    return count;
}
