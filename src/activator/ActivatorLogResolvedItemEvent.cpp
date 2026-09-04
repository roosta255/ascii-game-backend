#include "ActivatorLogResolvedItemEvent.hpp"
#include "LoggedEvent.hpp"
#include "MatchController.hpp"

bool ActivatorLogResolvedItemEvent::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        const ItemEnum item = activation.resolvedItem.orElse(ITEM_NIL);
        if (item == ITEM_NIL) return;

        req.controller.addLoggedEvent(activation, activation.room.roomId, LoggedEvent{
            event,
            { EventComponentKind::ROLE, (int)activation.character.role },
            {},
            { EventComponentKind::ITEM, (int)item },
            -1
        });
        result = true;
    });
    return result;
}
