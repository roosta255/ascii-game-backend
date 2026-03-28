#include "ActivatorShifter.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "EventFlyweight.hpp"
#include "ItemEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorShifter::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& room = activation.room;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    Wall& sourceWall = activation.room.getWall(direction);

    const Timestamp doorTime = activation.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            switch (sourceWall.door) {
                case DOOR_SHIFTER_INGRESS_KEYLESS:
                    // Wrapper spent the action and took the key; transition door state.
                    sourceWall.setDoor(DOOR_SHIFTER_INGRESS_KEYED, doorTime, activation.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                    neighborWall.setDoor(DOOR_SHIFTER_EGRESS_KEYED, doorTime, activation.isSkippingAnimations, neighborId, ANIMATION_SLIDE);
                    isSuccess = true;
                    controller.appendEventLog(activation, LoggedEvent{
                        EVENT_UNLOCK,
                        { EventComponentKind::ROLE, (int)subject.role },
                        { EventComponentKind::ITEM, (int)ITEM_KEY },
                        { EventComponentKind::DOOR, (int)DOOR_SHIFTER_INGRESS_KEYED },
                        (int)direction
                    });
                    return;
                case DOOR_SHIFTER_EGRESS_KEYED:
                    // GiveItem effect already gave the key; transition door state.
                    sourceWall.setDoor(DOOR_SHIFTER_EGRESS_KEYLESS, doorTime, activation.isSkippingAnimations, room.roomId, ANIMATION_CRUSH);
                    neighborWall.setDoor(DOOR_SHIFTER_INGRESS_KEYLESS, doorTime, activation.isSkippingAnimations, neighborId, ANIMATION_CRUSH);
                    isSuccess = true;
                    controller.appendEventLog(activation, LoggedEvent{
                        EVENT_LOCK,
                        { EventComponentKind::ROLE, (int)subject.role },
                        { EventComponentKind::ITEM, (int)ITEM_KEY },
                        { EventComponentKind::DOOR, (int)DOOR_SHIFTER_EGRESS_KEYLESS },
                        (int)direction
                    });
                    return;
                default:
                    codeset.addError(CODE_SHIFTER_ACTIVATION_ON_NON_INGRESS_SHIFTER);
                    return;
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);

    return isSuccess;
}
