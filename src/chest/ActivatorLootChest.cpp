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

        if (codeset.addFailure(!subject.isActor(codeset.error, controller.getTraitsComputed(subject.characterId).final))) {
            return;
        }

        if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
            return;
        }

        codeset.addFailure(!activation.targetCharacter().access([&](Character& containerChar) {
            codeset.addFailure(!match.dungeon.findChestByContainerId(containerChar.characterId, codeset.error).access([&](Chest& chest) {
                auto item = chest.inventory.items.getPointer(activation.targetItemSlot.orElse(-1));
                codeset.addFailure(!item.access([&](Item& chestItem) {
                    bool isLocked = false;
                    LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf){
                        isLocked = lf.isLocked;
                    });
                    if (codeset.addFailure(isLocked, CODE_LOOT_CHEST_IS_LOCKED)) {
                        controller.addRequestLoggedEvent(activation, LoggedEvent{
                            EVENT_CHEST_LOCKED,
                            { EventComponentKind::ROLE, (int)subject.role },
                            {},
                            { EventComponentKind::CHEST_LOCK, (int)chest.lock },
                            -1
                        });
                        return;
                    }

                    bool isTransferable = false;
                    chestItem.accessFlyweight([&](const ItemFlyweight& flyweight) {
                        isTransferable = flyweight.itemAttributes[TRAIT_ITEM_TRANSFERABLE].orElse(false);
                    });
                    if (codeset.addFailure(!isTransferable, CODE_LOOT_CHEST_ITEM_NOT_TRANSFERABLE)) return;

                    const ItemEnum type = chestItem.type;
                    if (codeset.addFailure(!controller.takeInventoryItem(chest.inventory, type, req.time, room.roomId, req.isSkippingAnimations))) return;
                    if (codeset.addFailure(!controller.giveInventoryItem(player.inventory, type, req.time, room.roomId, req.isSkippingAnimations))) return;
                    if (codeset.addFailure(!controller.takeCharacterAction(subject))) {
                        controller.addRequestLoggedEvent(activation, LoggedEvent{
                            EVENT_NO_ACTIONS,
                            { EventComponentKind::ROLE, (int)subject.role },
                            {}, {}, -1
                        });
                        return;
                    }
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
                }), CODE_LOOT_CHEST_ITEM_SLOT_NOT_SPECIFIED);
            }));
        }), CODE_INACCESSIBLE_TARGET_CHARACTER_ID);
    });
    return isSuccess;
}
