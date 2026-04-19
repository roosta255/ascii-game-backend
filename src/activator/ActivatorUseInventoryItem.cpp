#include "ActivatorUseInventoryItem.hpp"
#include "Codeset.hpp"
#include "ItemFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseInventoryItem::activate(ActivationContext& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = activation.room;
        auto& subject = activation.character;

        if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
            return;
        }

        codeset.addFailure(!activation.sourceItem.access([&](Item& item){
            codeset.addFailure(!item.accessFlyweight([&](const ItemFlyweight& flyweight){
                flyweight.useActivator.accessConst([&](const iActivator& activatorIntf){
                    codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
                });
            }), CODE_INACCESSIBLE_INVENTORY_FLYWEIGHT);
        }), CODE_INVENTORY_ITEM_IS_MISSING);
    });
    return isSuccess;
}
