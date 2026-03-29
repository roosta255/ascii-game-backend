#include "ActivatorGiveItem.hpp"
#include "Codeset.hpp"
#include "Inventory.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorGiveItem::activate(Activation& activation) const {
    auto& controller = activation.request->controller;
    auto& room = activation.request->room;
    auto& inventory = activation.request->player.inventory;
    const bool success = controller.giveInventoryItem(inventory, item, activation.request->time, room.roomId, activation.request->isSkippingAnimations);
    if (activation.request->codeset.addFailure(!success, CODE_INVENTORY_FAILED_TO_ACCEPT_ITEM)) return false;
    return true;
}
