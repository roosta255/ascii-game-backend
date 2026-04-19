#include "ActivatorUseChestLock.hpp"
#include "ActivationContext.hpp"
#include "Chest.hpp"
#include "Codeset.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseChestLock::activate(ActivationContext& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset    = req.codeset;

        codeset.addFailure(!activation.targetCharacter().access([&](Character& containerChar) {
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
    });
    return isSuccess;
}
