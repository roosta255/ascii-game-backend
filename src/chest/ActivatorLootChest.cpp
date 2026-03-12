#include "ActivatorCritterBite.hpp"
#include "ActivatorLootChest.hpp"
#include "Activation.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"

bool ActivatorLootChest::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& match = activation.match;
    auto& player = activation.player;
    auto& subject = activation.character;
    auto& room = activation.room;
    auto& item = activation.targetItem;

    if (codeset.addFailure(!subject.isActor(codeset.error, controller.getTraitsComputed(subject.characterId).final))) {
        return false;
    }

    if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!item.access([&](Item& chestItem) {
        codeset.addFailure(!activation.target.access([&](Character& containerChar) {
            codeset.addFailure(!match.dungeon.findChestByContainerId(containerChar.characterId, codeset.error).access([&](Chest& chest) {
                // Check lock
                bool isLocked = false;
                LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf){
                    isLocked = lf.isLocked;
                });
                if (codeset.addFailure(isLocked, CODE_LOOT_CHEST_IS_LOCKED)) return;

                const ItemEnum type = chestItem.type;
                if (codeset.addFailure(!controller.takeInventoryItem(chest.inventory, type, activation.time, room.roomId, activation.isSkippingAnimations))) return;
                if (codeset.addFailure(!controller.giveInventoryItem(player.inventory, type, activation.time, room.roomId, activation.isSkippingAnimations))) return;
                if (codeset.addFailure(!controller.takeCharacterAction(subject))) return;
                isSuccess = true;

                // Trigger critter bite if a critter guards this chest
                if (chest.critterCharacterId != -1) {
                    static ActivatorCritterBite critterBiteActivator;
                    Preactivation critterPreactivation{
                        .action = {
                            .characterId = chest.critterCharacterId,
                            .roomId = room.roomId,
                            .targetCharacterId = subject.characterId,
                        },
                        .playerId = player.account.toString(),
                        .isSkippingAnimations = activation.isSkippingAnimations,
                        .isSortingState = activation.isSortingState,
                        .time = activation.time
                    };
                    controller.activate(critterBiteActivator, critterPreactivation);
                }
            }));
        }), CODE_INACCESSIBLE_TARGET_CHARACTER_ID);
    }), CODE_LOOT_CHEST_ITEM_SLOT_NOT_SPECIFIED);

    return isSuccess;
}
