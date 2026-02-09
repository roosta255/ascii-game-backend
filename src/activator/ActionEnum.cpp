#include "ActionEnum.hpp"

const char* action_to_text(int index) {
    static const char* names[] = {
        #define ACTION_DECL( name_text, activation_intf_ ) #name_text,
        #include "Action.enum"
        #undef ACTION_DECL
    };
    if (index < 0) {
        return "ACTION_INDEX_NEGATIVE";
    }
    if (index >= ACTION_COUNT) {
        return "ACTION_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
