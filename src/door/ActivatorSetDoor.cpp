#include "ActivatorSetDoor.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorSetDoor::activate(Activation& activation) const {
    auto& codeset = activation.request->codeset;
    auto& room = activation.request->room;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    const Timestamp doorTime = activation.request->time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;
    room.getWall(direction).setDoor(door, doorTime, activation.request->isSkippingAnimations, room.roomId, animation);

    if (event.action != EVENT_NIL) {
        LoggedEvent toLog = event;
        toLog.actor     = { EventComponentKind::ROLE, (int)activation.character.role };
        toLog.target    = { EventComponentKind::DOOR, (int)door };
        toLog.direction = (int)direction;
        activation.request->controller.appendEventLog(activation, toLog);
    }
    return true;
}
