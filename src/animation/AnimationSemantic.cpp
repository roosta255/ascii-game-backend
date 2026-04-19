#include "AnimationSemantic.hpp"

const char* animation_semantic_to_text(int semantic) {
    switch (semantic) {
#define ANIMATION_SEMANTIC_DECL(name_, ...) case ANIMATION_SEMANTIC_##name_: return #name_;
#include "AnimationSemantic.enum"
#undef ANIMATION_SEMANTIC_DECL
        default: return "UNKNOWN";
    }
}

AnimationSemantic animation_semantic_flipped(AnimationSemantic semantic) {
    switch (semantic) {
#define ANIMATION_SEMANTIC_DECL(name_, flipped_) case ANIMATION_SEMANTIC_##name_: return flipped_;
#include "AnimationSemantic.enum"
#undef ANIMATION_SEMANTIC_DECL
        default: return ANIMATION_SEMANTIC_NONE;
    }
}