#include "LockEnum.hpp"

const char* action_to_text(int index) {
    static const char* names[] = {
        #define LOCK_DECL( name_, is_locked_, activation_intf_ ) #name_,
        #include "Lock.enum"
        #undef LOCK_DECL
    };
    if (index < 0) {
        return "LOCK_INDEX_NEGATIVE";
    }
    if (index >= LOCK_COUNT) {
        return "LOCK_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
