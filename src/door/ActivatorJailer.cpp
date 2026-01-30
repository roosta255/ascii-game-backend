#include "ActivatorJailer.hpp"
#include "Codeset.hpp"
#include "Character.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorJailer::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;
    auto& room = activation.room;

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(activation.character)) {
        return false;
    }

    Wall& sourceWall = room.getWall(activation.direction);

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            // Only at ingress keyless can we give a key
            if (controller.takeCharacterAction(subject)) {
                if (controller.takeInventoryItem(inventory, ITEM_KEY)) {
                    sourceWall.door = DOOR_JAILER_INGRESS_KEYED;
                    neighborWall.door = DOOR_JAILER_EGRESS_KEYED;
                    isSuccess = true;
                }
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);
    return isSuccess;
} 