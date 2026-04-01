#include "ActivatorChestLockKey.hpp"
#include "Activation.hpp"
#include "Chest.hpp"
#include "Codeset.hpp"
#include "Dungeon.hpp"
#include "Item.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"

bool ActivatorChestLockKey::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset    = req.codeset;
        auto& match      = req.match;
        auto& player     = req.player;
        auto& subject    = activation.character;
        auto& room       = req.room;

        const auto computed = controller.getTraitsComputed(subject.characterId).final;
        if (codeset.addFailure(!subject.isActor(codeset.error, computed))) return;
        if (codeset.addFailure(!subject.isKeyer(codeset.error, computed))) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NOT_KEYER,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        codeset.addFailure(!activation.targetCharacter().access([&](Character& containerChar) {
            codeset.addFailure(!match.dungeon.findChestByContainerId(containerChar.characterId, codeset.error).access([&](Chest& chest) {
                switch (chest.lock) {
                    case LOCK_KEY_CATALYST_CLOSED: {
                        if (codeset.addFailure(!controller.takeInventoryItem(player.inventory, ITEM_KEY, true))) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_MISSING_ITEM,
                                { EventComponentKind::ROLE, (int)subject.role },
                                {},
                                { EventComponentKind::ITEM, (int)ITEM_KEY },
                                -1
                            });
                            return;
                        }
                        if (codeset.addFailure(!controller.takeCharacterAction(subject))) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_NO_ACTIONS,
                                { EventComponentKind::ROLE, (int)subject.role },
                                {}, {}, -1
                            });
                            return;
                        }
                        chest.lock = LOCK_KEY_CATALYST_OPEN;
                        if (!req.isSkippingAnimations) {
                            Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Chest::MAX_KEYFRAMES>(chest.keyframes),
                                Keyframe::buildTransition(req.time, 300, room.roomId, ANIMATION_FALL, LOCK_KEY_CATALYST_CLOSED, LOCK_KEY_CATALYST_OPEN)
                            );
                        }
                        isSuccess = true;
                        break;
                    }
                    case LOCK_KEY_CONSUMER_CLOSED: {
                        if (codeset.addFailure(!controller.takeInventoryItem(player.inventory, ITEM_KEY, req.time, room.roomId, req.isSkippingAnimations))) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_MISSING_ITEM,
                                { EventComponentKind::ROLE, (int)subject.role },
                                {},
                                { EventComponentKind::ITEM, (int)ITEM_KEY },
                                -1
                            });
                            return;
                        }
                        if (codeset.addFailure(!controller.takeCharacterAction(subject))) {
                            controller.addRequestLoggedEvent(activation, LoggedEvent{
                                EVENT_NO_ACTIONS,
                                { EventComponentKind::ROLE, (int)subject.role },
                                {}, {}, -1
                            });
                            return;
                        }
                        chest.lock = LOCK_KEY_CONSUMER_OPEN;
                        if (!req.isSkippingAnimations) {
                            Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Chest::MAX_KEYFRAMES>(chest.keyframes),
                                Keyframe::buildTransition(req.time, 300, room.roomId, ANIMATION_FALL, LOCK_KEY_CONSUMER_CLOSED, LOCK_KEY_CONSUMER_OPEN)
                            );
                        }
                        isSuccess = true;
                        break;
                    }
                    default:
                        break;
                }
            }));
        }), CODE_INACCESSIBLE_TARGET_CHARACTER_ID);
    });
    return isSuccess;
}
