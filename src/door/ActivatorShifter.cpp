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
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& subject = activation.character;
        auto& room = req.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        Wall& sourceWall = room.getWall(direction);
        const Timestamp doorTime = req.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

        const bool isNeighborAccessed = req.match.dungeon.accessWallNeighbor(room, direction,
            [&](Wall& neighborWall, Room& neighbor, int neighborId) {
                switch (sourceWall.door) {
                    case DOOR_SHIFTER_INGRESS_KEYLESS:
                        sourceWall.setDoor(DOOR_SHIFTER_INGRESS_KEYED, doorTime, req.isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                        neighborWall.setDoor(DOOR_SHIFTER_EGRESS_KEYED, doorTime, req.isSkippingAnimations, neighborId, ANIMATION_SLIDE);
                        isSuccess = true;
                        controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
                            EVENT_UNLOCK,
                            { EventComponentKind::ROLE, (int)subject.role },
                            { EventComponentKind::ITEM, (int)ITEM_KEY },
                            { EventComponentKind::DOOR, (int)DOOR_SHIFTER_INGRESS_KEYED },
                            (int)direction
                        });
                        return;
                    case DOOR_SHIFTER_EGRESS_KEYED:
                        sourceWall.setDoor(DOOR_SHIFTER_EGRESS_KEYLESS, doorTime, req.isSkippingAnimations, room.roomId, ANIMATION_CRUSH);
                        neighborWall.setDoor(DOOR_SHIFTER_INGRESS_KEYLESS, doorTime, req.isSkippingAnimations, neighborId, ANIMATION_CRUSH);
                        isSuccess = true;
                        controller.addLoggedEvent(activation, room.roomId, LoggedEvent{
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
    });
    return isSuccess;
}
