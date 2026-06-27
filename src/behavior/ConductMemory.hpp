#pragma once

#include "BehaviorEnum.hpp"

// Per-conduct FSM state slot. Variable data shared across all conducts lives
// in Conduct::vars — see Conduct.hpp.
struct ConductMemory {
    BehaviorEnum state = BEHAVIOR_NIL;
};
