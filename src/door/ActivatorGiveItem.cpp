#include "ActivatorGiveItem.hpp"
#include "Codeset.hpp"
#include "Inventory.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorGiveItem::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& room = req.room;
        auto& inventory = req.player.inventory;
        const bool success = controller.giveInventoryItem(inventory, item, req.time, room.roomId, req.isSkippingAnimations);
        if (req.codeset.addFailure(!success, CODE_INVENTORY_FAILED_TO_ACCEPT_ITEM)) return;
        result = true;
    });
    return result;
}
