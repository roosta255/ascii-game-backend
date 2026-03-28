#include "ActivatorJailer.hpp"
#include "Codeset.hpp"
#include "Character.hpp"
#include "DoorEnum.hpp"
#include "EventFlyweight.hpp"
#include "ItemEnum.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorJailer::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& room = activation.room;
    auto& subject = activation.character;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    Wall& sourceWall = room.getWall(direction);

    const Timestamp doorTime = activation.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            // Wrapper spent the action and took the key; transition door state.
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
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    return isSuccess;
} 