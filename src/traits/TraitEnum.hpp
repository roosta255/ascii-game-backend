#pragma once

#include <string>

#define TRAIT_DECL( name_, type_ ) TRAIT_##name_,
#define TRAIT_MODIFIER_DECL( is_global_, modifier_ ) 
enum TraitEnum
{
#include "Trait.enum"
TRAIT_COUNT
};
#undef TRAIT_DECL
#undef TRAIT_MODIFIER_DECL

const char* action_to_text(int);

inline std::ostream& operator<<(std::ostream& os, TraitEnum action) {
    return os << std::string(action_to_text(action));
}
