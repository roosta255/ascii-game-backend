#pragma once

#include <ostream>

enum AnimationSemantic {
#define ANIMATION_SEMANTIC_DECL(name, ...) ANIMATION_SEMANTIC_##name,
#include "AnimationSemantic.enum"
#undef ANIMATION_SEMANTIC_DECL
    ANIMATION_SEMANTIC_COUNT
};

const char* animation_semantic_to_text(int semantic);

AnimationSemantic animation_semantic_flipped(AnimationSemantic semantic);

inline std::ostream& operator<<(std::ostream& os, AnimationSemantic semantic) {
    return os << animation_semantic_to_text((int)semantic);
}
