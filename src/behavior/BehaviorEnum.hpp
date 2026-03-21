#pragma once

#include <ostream>
#include <string>

#define BEHAVIOR_DECL(name_) BEHAVIOR_##name_,
enum BehaviorEnum {
#include "Behavior.enum"
    BEHAVIOR_COUNT
};
#undef BEHAVIOR_DECL

const char* behavior_to_text(int);

inline std::ostream& operator<<(std::ostream& os, BehaviorEnum b) {
    return os << behavior_to_text(b);
}
