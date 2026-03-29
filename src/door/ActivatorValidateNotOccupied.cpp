#include "ActivatorValidateNotOccupied.hpp"
#include "Cardinal.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorValidateNotOccupied::activate(Activation& activation) const {
    auto& codeset = activation.request->codeset;
    auto& room = activation.request->room;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    return activation.request->controller.validateSharedDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, direction);
}
