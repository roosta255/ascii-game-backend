#include "ActivatorShifter.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorShifter::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;
    auto& room = activation.room;

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    // checking occupancy even if keyless because a closed door should be occupied
    if (!controller.validateSharedDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, activation.direction)) {
        return false;
    }

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            switch (sourceWall.door) {
                case DOOR_SHIFTER_INGRESS_KEYLESS:
                    // Only at ingress keyless can we take a key
                    if (controller.takeCharacterAction(subject) && controller.takeInventoryItem(inventory, ITEM_KEY)) {
                        sourceWall.door = DOOR_SHIFTER_INGRESS_KEYED;
                        neighborWall.door = DOOR_SHIFTER_EGRESS_KEYED;
                        isSuccess = true;
                    }
                    return;
                case DOOR_SHIFTER_EGRESS_KEYED:
                    // Only at egress keyed can we give a key
                    if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_KEY)) {
                        sourceWall.door = DOOR_SHIFTER_EGRESS_KEYLESS;
                        neighborWall.door = DOOR_SHIFTER_INGRESS_KEYLESS;
                        isSuccess = true;
                    }
                    return;
                default:
                    codeset.addError(CODE_SHIFTER_ACTIVATION_ON_NON_INGRESS_SHIFTER);
                    return;
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);

    return isSuccess;
} 