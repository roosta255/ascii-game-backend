#include "ActivatorInactiveDoor.hpp"
#include "Match.hpp"

CodeEnum ActivatorInactiveDoor::activate(Activation& activation) const {
    return CODE_INACTIVE_DOOR;
} 