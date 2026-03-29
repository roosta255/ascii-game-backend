#include "ActivatorSetDoor.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorSetDoor::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& codeset = req.codeset;
        auto& room = req.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        const Timestamp doorTime = req.time + MatchController::BOUNCE_LOCK_ANIMATION_DURATION / 2;
        room.getWall(direction).setDoor(door, doorTime, req.isSkippingAnimations, room.roomId, animation);

        if (event.action != EVENT_NIL) {
            LoggedEvent toLog = event;
            toLog.actor     = { EventComponentKind::ROLE, (int)activation.character.role };
            toLog.target    = { EventComponentKind::DOOR, (int)door };
            toLog.direction = (int)direction;
            req.controller.addLoggedEvent(activation, room.roomId, toLog);
        }
        result = true;
    });
    return result;
}
