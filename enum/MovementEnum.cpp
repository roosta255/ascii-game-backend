#include "MovementEnum.hpp"

const char* movement_to_text(int index) {
    static const char* names[] = {
        #define MOVEMENT_DECL( name_text ) #name_text,
        #include "Movement.enum"
        #undef MOVEMENT_DECL
    };
    if (index < 0) {
        return "MOVEMENT_INDEX_NEGATIVE";
    }
    if (index >= MOVEMENT_COUNT) {
        return "MOVEMENT_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}