#include "AnimationEnum.hpp"

const char* animation_to_text(int index) {
    static const char* names[] = {
        #define ANIMATION_DECL( name_text ) #name_text,
        #include "Animation.enum"
        #undef ANIMATION_DECL
    };
    if (index < 0) {
        return "ANIMATION_INDEX_NEGATIVE";
    }
    if (index >= ANIMATION_COUNT) {
        return "ANIMATION_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
