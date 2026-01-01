#pragma once

#include <string>

#define ANIMATION_DECL( name ) ANIMATION_##name,
enum AnimationEnum
{
#include "Animation.enum"
ANIMATION_COUNT
};
#undef ANIMATION_DECL

const char* animation_to_text(int);

inline std::ostream& operator<<(std::ostream& os, AnimationEnum code) {
    return os << std::string(animation_to_text(code));
}
