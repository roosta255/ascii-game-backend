#include "ActivatorUseInventoryItem.hpp"
#include "Codeset.hpp"
#include "ItemFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorUseInventoryItem::activate(Activation& activation) const {
    auto& controller = activation.request->controller;
    auto& codeset = activation.request->codeset;
    auto& match = activation.request->match;
    auto& player = activation.request->player;
    auto& inventory = player.inventory;
    auto& room = activation.request->room;
    auto& subject = activation.character;

    if (codeset.addFailure(!controller.validateCharacterWithinRoom(subject.characterId, room.roomId), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!activation.sourceItem.access([&](Item& item){
        codeset.addFailure(!item.accessFlyweight([&](const ItemFlyweight& flyweight){
            flyweight.useActivator.accessConst([&](const iActivator& activatorIntf){
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            });
        }), CODE_INACCESSIBLE_INVENTORY_FLYWEIGHT);
    }), CODE_INVENTORY_ITEM_IS_MISSING);

    return isSuccess;
}
