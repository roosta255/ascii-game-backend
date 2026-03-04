#include "TraitEnum.hpp"

const char* action_to_text(int index) {
    static const char* names[] = {
        #define TRAIT_DECL( name_, type_ ) #name_,
        #define TRAIT_MODIFIER_DECL( is_global_, modifier_ ) 
        #include "Trait.enum"
        #undef TRAIT_DECL
        #undef TRAIT_MODIFIER_DECL
    };
    if (index < 0) {
        return "TRAIT_INDEX_NEGATIVE";
    }
    if (index >= TRAIT_COUNT) {
        return "TRAIT_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
