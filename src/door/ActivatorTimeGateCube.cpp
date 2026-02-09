#include "ActivatorTimeGateCube.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorTimeGateCube::activate(Activation& activation) const {
    MatchController& controller = activation.controller;
    Codeset& codeset = activation.codeset;
    Character& subject = activation.character;
    auto& inventory = activation.player.inventory;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = activation.room.getWall(direction);

    switch (sourceWall.door) {
        case DOOR_TIME_GATE_AWAKENED:
            // give cube from gate
            if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED)) {
                sourceWall.door = DOOR_TIME_GATE_EMPTY;
                return true;
            }
            break;
        case DOOR_TIME_GATE_DORMANT:
            // give cube from gate
            if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT)) {
                sourceWall.door = DOOR_TIME_GATE_EMPTY;
                return true;
            }
            break;
        case DOOR_TIME_GATE_EMPTY:
            // take cube to gate
            if (controller.takeCharacterAction(subject) && controller.takeInventoryItem(inventory, ITEM_CUBE_AWAKENED)) {
                sourceWall.door = DOOR_TIME_GATE_AWAKENED;
                return true;
            }
            break;
        default:
            codeset.addError(CODE_TIME_GATE_ACTIVATION_ON_NON_TIME_GATE);
            return false;
    }

    // switch makes this unreachable
    return false;
} 