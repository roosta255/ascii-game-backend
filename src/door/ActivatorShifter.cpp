#include "ActivatorShifter.hpp"
#include "Character.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

CodeEnum ActivatorShifter::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    if (!activation.character.isKeyer(result)) {
        return result;
    }

    const bool isKeying = sourceWall.door == DOOR_SHIFTER_EGRESS_KEYED;
    if (!activation.player.inventory.processDelta(ITEM_KEY, isKeying, result, true)) {
        return result;
    }

    int outCharacterId;

    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(activation.room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            switch (sourceWall.door) {
                case DOOR_SHIFTER_INGRESS_KEYLESS:
                    // Only at ingress keyless can we take a key
                    if (activation.controller.isDoorOccupied(activation.getRoomId(), CHANNEL_CORPOREAL, activation.direction, outCharacterId)) {
                        result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                        return;
                    }
                    if (activation.controller.isDoorOccupied(neighborId, CHANNEL_CORPOREAL, activation.direction.getFlip(), outCharacterId)) {
                        result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                        return;
                    }
                    if (activation.player.inventory.takeItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_SHIFTER_INGRESS_KEYED;
                        neighborWall.door = DOOR_SHIFTER_EGRESS_KEYED;
                        result = CODE_SUCCESS;
                    }
                    return;
                case DOOR_SHIFTER_EGRESS_KEYED:
                    // Only at egress keyed can we give a key
                    if (activation.player.inventory.giveItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_SHIFTER_EGRESS_KEYLESS;
                        neighborWall.door = DOOR_SHIFTER_INGRESS_KEYLESS;
                        result = CODE_SUCCESS;
                    }
                    return;
                default:
                    result = CODE_SHIFTER_ACTIVATION_ON_NON_INGRESS_SHIFTER;
                    return;
            }
        }
    );

    if (!isNeighborAccessed) {
        return CODE_INACCESSIBLE_NEIGHBORING_WALL;
    }

    return result;
} 