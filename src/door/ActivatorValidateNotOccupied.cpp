#include "ActivatorValidateNotOccupied.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "LoggedEvent.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorValidateNotOccupied::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& codeset = req.codeset;
        auto& room = activation.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        Wall* wallPtr = nullptr;
        activation.targetWall().access([&](Wall& w) { wallPtr = &w; });
        if (codeset.addFailure(!wallPtr, CODE_ACTIVATION_TARGET_NOT_SPECIFIED)) return;
        Wall& wall = *wallPtr;

        result = req.controller.validateSharedDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, direction);
        if (codeset.addFailure(!result, CODE_SHARED_DOORWAY_OCCUPIED_BY_CHARACTER)) {
            int occupyingCharacterId = -1;
            if (!req.controller.isDoorOccupied(room.roomId, CHANNEL_CORPOREAL, direction, occupyingCharacterId)) {
                req.controller.isDoorOccupied(wall.adjacent, CHANNEL_CORPOREAL, direction.getFlip(), occupyingCharacterId);
            }
            if (occupyingCharacterId != -1) {
                RoleEnum occupyingRole = ROLE_EMPTY;
                CodeEnum charError = CODE_UNKNOWN_ERROR;
                req.match.getCharacter(occupyingCharacterId, charError).access([&](Character& c) {
                    occupyingRole = c.role;
                });
                req.controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_OCCUPIED_DOORWAY,
                    { EventComponentKind::ROLE, (int)occupyingRole },
                    {},
                    { EventComponentKind::DOOR, (int)wall.door },
                    direction.getIndex()
                });
            }
        }
    });
    return result;
}
