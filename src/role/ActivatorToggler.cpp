#include "ActivatorToggler.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorToggler::activate(Activation& activation) const {
    if (activation.codeset.addFailure(activation.target.isEmpty(), CODE_TARGET_CHARACTER_MISSING)) {
        return false;
    }
    bool isSuccess = false;
    activation.target.access([&](Character& target) {
        if (activation.controller.takeCharacterAction(target)) {
            activation.match.dungeon.toggleDoors();
            isSuccess = true;
        }
    });
    return isSuccess;
}