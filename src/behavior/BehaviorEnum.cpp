#include "BehaviorEnum.hpp"

const char* behavior_to_text(int index) {
    static const char* names[] = {
        #define BEHAVIOR_DECL(name_) #name_,
        #include "Behavior.enum"
        #undef BEHAVIOR_DECL
    };
    if (index < 0) {
        return "BEHAVIOR_INDEX_NEGATIVE";
    }
    if (index >= BEHAVIOR_COUNT) {
        return "BEHAVIOR_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
