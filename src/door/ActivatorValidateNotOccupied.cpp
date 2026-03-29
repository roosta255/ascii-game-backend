#include "ActivatorValidateNotOccupied.hpp"
#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorValidateNotOccupied::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& codeset = req.codeset;
        auto& room = req.room;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        result = req.controller.validateSharedDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, direction);
    });
    return result;
}
