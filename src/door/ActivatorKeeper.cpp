#include "ActivatorKeeper.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorKeeper::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    activation.match.dungeon.accessWall(activation.room, activation.direction,
        [&](Cell& sourceCell, Cell& neighborCell, Room& neighborRoom) {
            Wall& sourceWall = activation.room.getWall(activation.direction);
            Wall& neighborWall = neighborRoom.getWall(activation.direction.getFlip());

            const bool isKeying = sourceWall.door == DOOR_KEEPER_INGRESS_KEYLESS;
            if (!activation.character.isKeyer(result, isKeying)) {
                return;
            }

            switch (sourceWall.door) {
                case DOOR_KEEPER_INGRESS_KEYED:
                    if (activation.match.containsCellCharacter(sourceCell) || activation.match.containsCellCharacter(neighborCell)) {
                        result = CODE_DOORWAY_OCCUPIED_BY_CHARACTER;
                        return;
                    }
                    if (activation.character.giveKey(result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_KEEPER_INGRESS_KEYLESS;
                        neighborWall.door = DOOR_KEEPER_EGRESS_KEYLESS;
                        result = CODE_SUCCESS;
                    }
                    return;
                case DOOR_KEEPER_INGRESS_KEYLESS:
                    if (activation.character.takeKey(result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_KEEPER_INGRESS_KEYED;
                        neighborWall.door = DOOR_KEEPER_EGRESS_KEYED;
                        result = CODE_SUCCESS;
                    }
                    return;
                default:
                    result = CODE_KEEPER_ACTIVATION_ON_NON_INGRESS_KEEPER;
                    return;
            }
        },
        [&](Cell& sourceCell) {
            result = CODE_INACCESSIBLE_NEIGHBORING_WALL;
        }
    );

    return result;
} 