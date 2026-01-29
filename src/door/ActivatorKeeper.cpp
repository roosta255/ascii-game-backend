#include "ActivatorKeeper.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorKeeper::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(activation.character)) {
        return false;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    // checking occupancy even if keyless because a closed door should be occupied
    if (!controller.validateSharedDoorNotOccupied(activation.getRoomId(), CHANNEL_CORPOREAL, activation.direction)) {
        return false;
    }

    bool isSuccess = false;
    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(activation.room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            switch (sourceWall.door) {
                case DOOR_KEEPER_INGRESS_KEYED:
                    if (controller.takeCharacterAction(subject)) {
                        if (controller.giveInventoryItem(inventory, ITEM_KEY)) {
                            sourceWall.door = DOOR_KEEPER_INGRESS_KEYLESS;
                            neighborWall.door = DOOR_KEEPER_EGRESS_KEYLESS;
                            isSuccess = true;
                        }
                    }
                    return;
                case DOOR_KEEPER_INGRESS_KEYLESS:
                    if (controller.takeCharacterAction(subject)) {
                        if (controller.takeInventoryItem(inventory, ITEM_KEY)) {
                            sourceWall.door = DOOR_KEEPER_INGRESS_KEYED;
                            neighborWall.door = DOOR_KEEPER_EGRESS_KEYED;
                            isSuccess = true;
                        }
                    }
                    return;
                default:
                    codeset.addError(CODE_KEEPER_ACTIVATION_ON_NON_INGRESS_KEEPER);
                    return;
            }
        }
    );

    codeset.addFailure(!isNeighborAccessed, CODE_INACCESSIBLE_NEIGHBORING_WALL);

    return isSuccess;
} 