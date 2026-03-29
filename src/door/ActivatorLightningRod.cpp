#include "ActivatorLightningRod.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Inventory.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorLightningRod::activate(Activation& activation) const {
    auto& controller = activation.request->controller;
    auto& codeset = activation.request->codeset;
    auto& subject = activation.character;
    auto& inventory = activation.request->player.inventory;
    auto& room = activation.request->room;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    // Check if character is actor
    if (!activation.request->controller.isCharacterKeyerValidation(subject)) {
        return false;
    }

    Wall& sourceWall = room.getWall(direction);

    switch (sourceWall.door) {
        case DOOR_LIGHTNING_ROD_AWAKENED:
            // give cube from rod
            if (controller.takeCharacterAction(subject)) {
                // it's fine to perform the inventory check last because failures arent saved
                if (controller.giveInventoryItem(inventory, ITEM_CUBE_AWAKENED, activation.request->time, room.roomId, activation.request->isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, activation.request->time, activation.request->isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                    return true;
                }
            }
            break;
        case DOOR_LIGHTNING_ROD_DORMANT:
            // give cube from rod
            if (controller.takeCharacterAction(subject) && controller.giveInventoryItem(inventory, ITEM_CUBE_DORMANT, activation.request->time, room.roomId, activation.request->isSkippingAnimations)) {
                sourceWall.setDoor(DOOR_LIGHTNING_ROD_EMPTY, activation.request->time, activation.request->isSkippingAnimations, room.roomId, ANIMATION_SLIDE);
                return true;
            }
            break;
        case DOOR_LIGHTNING_ROD_EMPTY:
            // take cube for rod
            if (controller.takeCharacterAction(subject)) {
                if (controller.takeInventoryItem(inventory, ITEM_CUBE_DORMANT, activation.request->time, room.roomId, activation.request->isSkippingAnimations)) {
                    sourceWall.setDoor(DOOR_LIGHTNING_ROD_AWAKENED, activation.request->time, activation.request->isSkippingAnimations, room.roomId, ANIMATION_CRUSH);
                    return true;
                }
            }
            break;
        default:
            codeset.addError(CODE_LIGHTNING_ROD_ACTIVATION_ON_NON_LIGHTNING_ROD);
    }

    // switch never reaches this
    return false;
}
