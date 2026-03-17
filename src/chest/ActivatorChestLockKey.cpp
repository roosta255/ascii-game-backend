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
    auto& controller = activation.controller;
    auto& codeset    = activation.codeset;
    auto& match      = activation.match;
    auto& player     = activation.player;
    auto& subject    = activation.character;
    auto& room       = activation.room;

    const auto computed = controller.getTraitsComputed(subject.characterId).final;
    if (codeset.addFailure(!subject.isActor(codeset.error, computed))) return false;
    if (codeset.addFailure(!subject.isKeyer(codeset.error, computed))) return false;

    bool isSuccess = false;
    codeset.addFailure(!activation.target.access([&](Character& containerChar) {
        codeset.addFailure(!match.dungeon.findChestByContainerId(containerChar.characterId, codeset.error).access([&](Chest& chest) {
            switch (chest.lock) {
                case LOCK_KEY_CATALYST_CLOSED: {
                    // Requires a key but does not consume it
                    if (codeset.addFailure(!controller.takeInventoryItem(player.inventory, ITEM_KEY, true))) return;
                    if (codeset.addFailure(!controller.takeCharacterAction(subject))) return;
                    chest.lock = LOCK_KEY_CATALYST_OPEN;
                    if (!activation.isSkippingAnimations) {
                        Keyframe::insertKeyframe(
                            Rack<Keyframe>::buildFromArray<Chest::MAX_KEYFRAMES>(chest.keyframes),
                            Keyframe::buildTransition(activation.time, 300, activation.room.roomId, ANIMATION_FALL, LOCK_KEY_CATALYST_CLOSED, LOCK_KEY_CATALYST_OPEN)
                        );
                    }
                    isSuccess = true;
                    break;
                }
                case LOCK_KEY_CONSUMER_CLOSED: {
                    // Requires and consumes a key
                    if (codeset.addFailure(!controller.takeInventoryItem(player.inventory, ITEM_KEY, activation.time, activation.room.roomId, activation.isSkippingAnimations))) return;
                    if (codeset.addFailure(!controller.takeCharacterAction(subject))) return;
                    chest.lock = LOCK_KEY_CONSUMER_OPEN;
                    if (!activation.isSkippingAnimations) {
                        Keyframe::insertKeyframe(
                            Rack<Keyframe>::buildFromArray<Chest::MAX_KEYFRAMES>(chest.keyframes),
                            Keyframe::buildTransition(activation.time, 300, activation.room.roomId, ANIMATION_FALL, LOCK_KEY_CONSUMER_CLOSED, LOCK_KEY_CONSUMER_OPEN)
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

    return isSuccess;
}
