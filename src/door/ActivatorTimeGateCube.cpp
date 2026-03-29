#include "ActivatorTimeGateCube.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorTimeGateCube::activate(Activation& activation) const {
    MatchController& controller = activation.request->controller;
    Codeset& codeset = activation.request->codeset;
    Character& subject = activation.character;
    auto& inventory = activation.request->player.inventory;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Check if character can use keys
    if (!controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = activation.request->room.getWall(direction);

    const int roomId = activation.request->room.roomId;
    switch (sourceWall.door) {
        case DOOR_TIME_GATE_AWAKENED:
            // give cube from gate
            if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, activation.request->time, roomId, activation.request->isSkippingAnimations)) {
                sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, activation.request->time, activation.request->isSkippingAnimations, roomId, ANIMATION_SLIDE);
                return true;
            }
            break;
        case DOOR_TIME_GATE_DORMANT:
            // give cube from gate
            if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, activation.request->time, roomId, activation.request->isSkippingAnimations)) {
                sourceWall.setDoor(DOOR_TIME_GATE_EMPTY, activation.request->time, activation.request->isSkippingAnimations, roomId, ANIMATION_SLIDE);
                return true;
            }
            break;
        case DOOR_TIME_GATE_EMPTY:
            // take cube to gate
            if (controller.takeCharacterAction(subject) && controller.takeInventoryItem(inventory, ITEM_CUBE_AWAKENED, activation.request->time, roomId, activation.request->isSkippingAnimations)) {
                sourceWall.setDoor(DOOR_TIME_GATE_AWAKENED, activation.request->time, activation.request->isSkippingAnimations, roomId, ANIMATION_CRUSH);
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
