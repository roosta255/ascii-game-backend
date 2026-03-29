#include "ActivatorSetSharedDoors.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorSetSharedDoors::activate(Activation& activation) const {
    auto& codeset = activation.request->codeset;
    auto& room = activation.request->room;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    const Timestamp doorTime = activation.request->time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;
    Wall& sourceWall = room.getWall(direction);

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.request->match.dungeon.accessWallNeighbor(room, direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            sourceWall.setDoor(sourceDoor, doorTime, activation.request->isSkippingAnimations, room.roomId, animation);
            neighborWall.setDoor(neighborDoor, doorTime, activation.request->isSkippingAnimations, neighborId, animation);
            isSuccess = true;
            if (event.action != EVENT_NIL) {
                LoggedEvent toLog = event;
                toLog.actor     = { EventComponentKind::ROLE, (int)activation.character.role };
                toLog.target    = { EventComponentKind::DOOR, (int)sourceDoor };
                toLog.direction = (int)direction;
                activation.request->controller.appendEventLog(activation, toLog);
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    return isSuccess;
}
