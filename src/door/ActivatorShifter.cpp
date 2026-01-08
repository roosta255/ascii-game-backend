#include "ActivatorShifter.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorShifter::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    activation.match.dungeon.accessWall(activation.room, activation.direction,
        [&](Cell& sourceCell, Cell& neighborCell, Room& neighborRoom) {
            Wall& sourceWall = activation.room.getWall(activation.direction);
            Wall& neighborWall = neighborRoom.getWall(activation.direction.getFlip());

            if (!activation.character.isKeyer(result)) {
                return;
            }

            const bool isKeying = sourceWall.door == DOOR_SHIFTER_EGRESS_KEYED;
            if (!activation.player.inventory.processDelta(ITEM_KEY, isKeying, result, true)) {
                return;
            }

            switch (sourceWall.door) {
                case DOOR_SHIFTER_INGRESS_KEYLESS:
                    // Only at ingress keyless can we take a key
                    if (activation.match.containsCellCharacter(sourceCell) || activation.match.containsCellCharacter(neighborCell)) {
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
        },
        [&](Cell& sourceCell) {
            result = CODE_INACCESSIBLE_NEIGHBORING_WALL;
        }
    );

    return result;
} 