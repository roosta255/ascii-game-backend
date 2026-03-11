#include "ActivatorUseChestLock.hpp"
#include "Activation.hpp"
#include "Chest.hpp"
#include "Codeset.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseChestLock::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset    = activation.codeset;

    bool isSuccess = false;
    codeset.addFailure(!activation.target.access([&](Character& containerChar) {
        codeset.addFailure(!controller.getChestByContainerId(containerChar.characterId).access([&](Chest& chest) {
            codeset.addFailure(!LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& flyweight) {
                if (!flyweight.activator.accessConst([&](const iActivator& lockActivator) {
                    codeset.addFailure(!(isSuccess = lockActivator.activate(activation)));
                })) {
                    codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
                }
            }));
        }), CODE_LOOT_CHEST_NOT_FOUND);
    }), CODE_INACCESSIBLE_TARGET_CHARACTER_ID);

    return isSuccess;
}
