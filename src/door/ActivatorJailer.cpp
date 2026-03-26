#include "ActivatorJailer.hpp"
#include "Codeset.hpp"
#include "Character.hpp"
#include "DoorEnum.hpp"
#include "EventFlyweight.hpp"
#include "ItemEnum.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorJailer::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = room.getWall(direction);

    const Timestamp doorTime = activation.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

    if (!activation.isSkippingAnimations) {
        if (subject.location.type == LOCATION_FLOOR) {
            Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, subject.location.data, direction));
        } else if (subject.location.type == LOCATION_DOOR) {
            Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, (Cardinal)subject.location.data, direction));
        }
    }

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            // Only at ingress keyless can we give a key
            if (controller.takeCharacterAction(subject)) {
                if (controller.takeInventoryItem(inventory, ITEM_KEY, activation.time, room.roomId, activation.isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_JAILER_INGRESS_KEYED, doorTime, activation.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                    neighborWall.setDoor(DOOR_JAILER_EGRESS_KEYED, doorTime, activation.isSkippingAnimations, neighborId, ANIMATION_SLIDE);
                    isSuccess = true;
                    controller.appendEventLog(activation, LoggedEvent{
                        EVENT_UNLOCK,
                        { EventComponentKind::ROLE, (int)subject.role },
                        { EventComponentKind::ITEM, (int)ITEM_KEY },
                        { EventComponentKind::DOOR, (int)DOOR_JAILER_INGRESS_KEYED },
                        (int)direction
                    });
                }
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    return isSuccess;
} 