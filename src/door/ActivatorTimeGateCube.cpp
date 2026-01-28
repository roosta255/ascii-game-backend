#include "ActivatorTimeGateCube.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorTimeGateCube::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    auto& inventory = activation.player.inventory;

    switch (sourceWall.door) {
        case DOOR_TIME_GATE_AWAKENED:
            // give cube from rod
            if (inventory.giveItem(ITEM_CUBE_AWAKENED, result) && activation.character.takeAction(result)) {
                sourceWall.door = DOOR_TIME_GATE_EMPTY;
                return CODE_SUCCESS;
            }
            break;
        case DOOR_TIME_GATE_DORMANT:
            // give cube from rod
            if (inventory.giveItem(ITEM_CUBE_DORMANT, result) && activation.character.takeAction(result)) {
                sourceWall.door = DOOR_TIME_GATE_EMPTY;
                return CODE_SUCCESS;
            }
            break;
        case DOOR_TIME_GATE_EMPTY:
            // take cube for rod
            if (inventory.takeItem(ITEM_CUBE_AWAKENED, result) && activation.character.takeAction(result)) {
                sourceWall.door = DOOR_TIME_GATE_AWAKENED;
                return CODE_SUCCESS;
            }
            break;
        default:
            return CODE_TIME_GATE_ACTIVATION_ON_NON_TIME_GATE;
    }

    return result;
} 