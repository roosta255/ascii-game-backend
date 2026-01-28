#include "ActivatorKeeper.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

CodeEnum ActivatorKeeper::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    if (!activation.character.isKeyer(result)) {
        return result;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    const bool isKeying = sourceWall.door == DOOR_KEEPER_INGRESS_KEYED;
    if (!activation.player.inventory.processDelta(ITEM_KEY, isKeying, result, true)) {
        return result;
    }

    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(activation.room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            switch (sourceWall.door) {
                case DOOR_KEEPER_INGRESS_KEYED:
                    if (activation.controller.isDoorOccupied(activation.getRoomId(), CHANNEL_CORPOREAL, activation.direction, outCharacterId)) {
                        result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                        return;
                    }
                    if (activation.controller.isDoorOccupied(neighborId, CHANNEL_CORPOREAL, activation.direction.getFlip(), outCharacterId)) {
                        result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                        return;
                    }
                    if (activation.player.inventory.giveItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_KEEPER_INGRESS_KEYLESS;
                        neighborWall.door = DOOR_KEEPER_EGRESS_KEYLESS;
                        result = CODE_SUCCESS;
                    }
                    return;
                case DOOR_KEEPER_INGRESS_KEYLESS:
                    if (activation.player.inventory.takeItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_KEEPER_INGRESS_KEYED;
                        neighborWall.door = DOOR_KEEPER_EGRESS_KEYED;
                        result = CODE_SUCCESS;
                    }
                    return;
                default:
                    result = CODE_KEEPER_ACTIVATION_ON_NON_INGRESS_KEEPER;
                    return;
            }
        }
    );

    if (!isNeighborAccessed) {
        return CODE_INACCESSIBLE_NEIGHBORING_WALL;
    }

    return result;
} 