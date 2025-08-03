#include "ActivatorToggler.hpp"
#include "Match.hpp"

CodeEnum ActivatorToggler::activate(Activation& activation) const {
    CodeEnum result = CODE_UNKNOWN_ERROR;
    if (activation.target.isEmpty()) {
        return CODE_TARGET_CHARACTER_MISSING;
    }
    if (!activation.target.access([&](Character& target) {
        if (target.isActor(result) && target.takeAction(result)) {
            activation.match.dungeon.toggleDoors();
            result = CODE_SUCCESS;
        }
    })) {
        return result;
    }
    return result;
}