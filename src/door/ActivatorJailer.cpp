#include "ActivatorJailer.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
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

    activation.match.dungeon.accessWall(activation.room, activation.direction,
        [&](Cell& sourceCell, Cell& neighborCell, Room& neighborRoom) {
            Wall& sourceWall = activation.room.getWall(activation.direction);
            Wall& neighborWall = neighborRoom.getWall(activation.direction.getFlip());

            if (!activation.character.isKeyer(result)) {
                return;
            }

            // Only at ingress keyless can we give a key
            if (activation.match.containsCellCharacter(sourceCell) || activation.match.containsCellCharacter(neighborCell)) {
                result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                return;
            }
            if (activation.player.inventory.takeItem(ITEM_KEY, result) && activation.character.takeAction(result)) {
                sourceWall.door = DOOR_JAILER_INGRESS_KEYED;
                neighborWall.door = DOOR_JAILER_EGRESS_KEYED;
                result = CODE_SUCCESS;
            }
        },
        [&](Cell& sourceCell) {
            result = CODE_INACCESSIBLE_NEIGHBORING_WALL;
        }
    );

    return result;
} 