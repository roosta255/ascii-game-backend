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
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = room.getWall(direction);

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, direction,
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