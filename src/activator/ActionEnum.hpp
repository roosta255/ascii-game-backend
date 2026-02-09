#pragma once

#include <string>

#define ACTION_DECL( name, activation_intf_ ) ACTION_##name,
enum ActionEnum
{
#include "Action.enum"
ACTION_COUNT
};
#undef ACTION_DECL

const char* action_to_text(int);

inline std::ostream& operator<<(std::ostream& os, ActionEnum action) {
    return os << std::string(action_to_text(action));
}
