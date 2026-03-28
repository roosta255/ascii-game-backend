#include "ActivatorGiveItem.hpp"
#include "Inventory.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorGiveItem::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& room = activation.room;
    auto& inventory = activation.player.inventory;
    return controller.giveInventoryItem(inventory, item, activation.time, room.roomId, activation.isSkippingAnimations);
}
