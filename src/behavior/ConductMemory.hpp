#pragma once

#include "BehaviorEnum.hpp"

struct ConductMemory {
    int targetCharacterId = -1;
    int targetObjectId    = -1;
    int targetRoomId      = -1;
    BehaviorEnum state    = BEHAVIOR_NIL;
};
