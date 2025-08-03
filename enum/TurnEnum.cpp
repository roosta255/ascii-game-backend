#include "TurnEnum.hpp"

const char* turn_to_text(int index) {
    static const char* names[] = {
        #define TURN_DECL( name ) #name,
        #include "Turn.enum"
        #undef TURN_DECL
    };
    if (index < 0) {
        return "TURN_INDEX_NEGATIVE";
    }
    if (index >= TURN_COUNT) {
        return "TURN_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}

bool text_to_turn(const std::string& text, TurnEnum& output) {
    output = TURN_PREGAME_TITAN_ONLY;  // Default value
    static const char* names[] = {
        #define TURN_DECL( name ) #name,
        #include "Turn.enum"
        #undef TURN_DECL
    };
    for (int i = 0; i < TURN_COUNT; i++) {
        if (text == names[i]) {
            output = static_cast<TurnEnum>(i);
            return true;
        }
    }
    return false;
} 