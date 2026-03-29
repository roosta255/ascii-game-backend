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
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = req.room;
        auto& subject = activation.character;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        Wall& sourceWall = room.getWall(direction);
        const Timestamp doorTime = req.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

        const bool isNeighborAccessed = req.match.dungeon.accessWallNeighbor(room, direction,
            [&](Wall& neighborWall, Room& neighbor, int neighborId) {
                sourceWall.setDoor(DOOR_JAILER_INGRESS_KEYED, doorTime, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                neighborWall.setDoor(DOOR_JAILER_EGRESS_KEYED, doorTime, req.isSkippingAnimations, neighborId, ANIMATION_SLIDE);
                isSuccess = true;
                controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
                    EVENT_UNLOCK,
                    { EventComponentKind::ROLE, (int)subject.role },
                    { EventComponentKind::ITEM, (int)ITEM_KEY },
                    { EventComponentKind::DOOR, (int)DOOR_JAILER_INGRESS_KEYED },
                    (int)direction
                });
            }
        );

        codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    });
    return isSuccess;
}
