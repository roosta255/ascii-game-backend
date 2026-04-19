#include "AnimationType.hpp"

const char* animation_type_to_text(int type) {
    switch (type) {
        case ANIMATION_TYPE_NIL: return "NIL";
#define ANIMATION_TYPE_DECL(name_) case ANIMATION_TYPE_##name_: return #name_;
#include "AnimationType.enum"
#undef ANIMATION_TYPE_DECL
#define ANIMATION_SEMANTIC_DECL(name_, ...) case ANIMATION_TYPE_SEMANTIC_##name_: return "SEMANTIC_" #name_;
#include "AnimationSemantic.enum"
#undef ANIMATION_SEMANTIC_DECL
        default: return "UNKNOWN";
    }
}

AnimationType semantic_to_type(AnimationSemantic semantic) {
    return AnimationType(ANIMATION_TYPE_COUNT + (int)semantic);
}
