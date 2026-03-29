#include "ActionFlyweight.hpp"
#include "ActivatorCritterBite.hpp"
#include "ActivatorLootChest.hpp"
#include "Activation.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Dungeon.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"

bool ActivatorLootChest::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& match = req.match;
        auto& player = req.player;
        auto& subject = activation.character;
        auto& room = req.room;
        auto& item = activation.targetItem;

        if (codeset.addFailure(!subject.isActor(codeset.error, controller.getTraitsComputed(subject.characterId).final))) {
            return;
        }

        if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
            return;
        }

        codeset.addFailure(!item.access([&](Item& chestItem) {
            codeset.addFailure(!activation.target.access([&](Character& containerChar) {
                codeset.addFailure(!match.dungeon.findChestByContainerId(containerChar.characterId, codeset.error).access([&](Chest& chest) {
                    bool isLocked = false;
                    LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf){
                        isLocked = lf.isLocked;
                    });
                    if (codeset.addFailure(isLocked, CODE_LOOT_CHEST_IS_LOCKED)) return;

                    bool isTransferable = false;
                    chestItem.accessFlyweight([&](const ItemFlyweight& flyweight) {
                        isTransferable = flyweight.itemAttributes[TRAIT_ITEM_TRANSFERABLE].orElse(false);
                    });
                    if (codeset.addFailure(!isTransferable, CODE_LOOT_CHEST_ITEM_NOT_TRANSFERABLE)) return;

                    const ItemEnum type = chestItem.type;
                    if (codeset.addFailure(!controller.takeInventoryItem(chest.inventory, type, req.time, room.roomId, req.isSkippingAnimations))) return;
                    if (codeset.addFailure(!controller.giveInventoryItem(player.inventory, type, req.time, room.roomId, req.isSkippingAnimations))) return;
                    if (codeset.addFailure(!controller.takeCharacterAction(subject))) return;
                    isSuccess = true;
                    controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
                        EVENT_LOOT_CHEST,
                        { EventComponentKind::ROLE, (int)subject.role },
                        {},
                        { EventComponentKind::ITEM, (int)type },
                        -1
                    });

                    chest.inventory.accessItems(ITEM_CRITTER, [&](const Item& critterItem) {
                        const auto critterCharacterId = critterItem.stacks;
                        Preactivation critterPreactivation{
                            .action = {
                                .type = ACTION_CRITTER_BITE,
                                .characterId = critterCharacterId,
                                .roomId = room.roomId,
                                .targetCharacterId = subject.characterId,
                            },
                            .playerId = player.account.toString(),
                            .isSkippingAnimations = req.isSkippingAnimations,
                            .isSortingState = activation.isSortingState,
                            .time = req.time
                        };
                        controller.activate(critterPreactivation);
                    });
                }));
            }), CODE_INACCESSIBLE_TARGET_CHARACTER_ID);
        }), CODE_LOOT_CHEST_ITEM_SLOT_NOT_SPECIFIED);
    });
    return isSuccess;
}
