#pragma once

#include <string>

#define LOCK_DECL( name_, is_locked_, activation_intf_ ) LOCK_##name_,
enum LockEnum
{
#include "Lock.enum"
LOCK_COUNT
};
#undef LOCK_DECL

const char* action_to_text(int);

inline std::ostream& operator<<(std::ostream& os, LockEnum action) {
    return os << std::string(action_to_text(action));
}
