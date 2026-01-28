#include "ActivatorJailer.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorJailer::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    if (!activation.player.inventory.takeItem(ITEM_KEY, result, true)) {
        return result;
    }

    if (!activation.character.isKeyer(result)) {
        return result;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    const bool isNeighborAccessed = activation.match.dungeon.accessWallNeighbor(activation.room, activation.direction,
        [&](Wall& neighborWall, Room& neighbor, int neighborId) {
            int outCharacterId;
            // Only at ingress keyless can we give a key
            if (activation.controller.isDoorOccupied(activation.getRoomId(), CHANNEL_CORPOREAL, activation.direction, outCharacterId)) {
                result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                return;
            }
            if (activation.controller.isDoorOccupied(neighborId, CHANNEL_CORPOREAL, activation.direction.getFlip(), outCharacterId)) {
                result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                return;
            }
            if (activation.player.inventory.takeItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                sourceWall.door = DOOR_JAILER_INGRESS_KEYED;
                neighborWall.door = DOOR_JAILER_EGRESS_KEYED;
                result = CODE_SUCCESS;
            }
        }
    );

    if (!isNeighborAccessed) {
        return CODE_INACCESSIBLE_NEIGHBORING_WALL;
    }

    return result;
} 