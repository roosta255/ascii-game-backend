#include "ActivatorSetSharedDoors.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorSetSharedDoors::activate(ActivationContext& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& codeset = req.codeset;
        auto& room = activation.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        const Timestamp doorTime = req.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;

        Wall* sourceWallPtr = nullptr;
        activation.targetWall().access([&](Wall& w) { sourceWallPtr = &w; });
        if (codeset.addFailure(!sourceWallPtr, CODE_ACTIVATION_TARGET_NOT_SPECIFIED)) return;
        Wall& sourceWall = *sourceWallPtr;

        const bool isNeighborAccessed = req.match.dungeon.accessWallNeighbor(room, direction,
            [&](Wall& neighborWall, Room& neighbor, int neighborId) {
                sourceWall.setDoor(sourceDoor, doorTime, req.isSkippingAnimations, room.roomId, animation);
                neighborWall.setDoor(neighborDoor, doorTime, req.isSkippingAnimations, neighborId, animation);
                isSuccess = true;
                if (event.action != EVENT_NIL) {
                    LoggedEvent toLog = event;
                    toLog.actor     = { EventComponentKind::ROLE, (int)activation.character.role };
                    toLog.target    = { EventComponentKind::DOOR, (int)sourceDoor };
                    toLog.direction = (int)direction;
                    req.controller.addLoggedEvent(activation, room.roomId, toLog);
                }
            }
        );

        codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    });
    return isSuccess;
}
