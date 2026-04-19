#pragma once

#include <ostream>

#include "AnimationSemantic.hpp"

enum AnimationType {
    ANIMATION_TYPE_NIL,
#define ANIMATION_TYPE_DECL(name) ANIMATION_TYPE_##name,
#include "AnimationType.enum"
#undef ANIMATION_TYPE_DECL
    ANIMATION_TYPE_COUNT,
#define ANIMATION_SEMANTIC_DECL(name, ...) ANIMATION_TYPE_SEMANTIC_##name = ANIMATION_TYPE_COUNT + ANIMATION_SEMANTIC_##name,
#include "AnimationSemantic.enum"
#undef ANIMATION_SEMANTIC_DECL
    ANIMATION_TYPE_TOTAL_COUNT = ANIMATION_TYPE_COUNT + ANIMATION_SEMANTIC_COUNT
};

AnimationType semantic_to_type(AnimationSemantic semantic);
const char* animation_type_to_text(int type);

inline std::ostream& operator<<(std::ostream& os, AnimationType type) {
    return os << animation_type_to_text((int)type);
}
