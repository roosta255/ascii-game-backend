#include "ActivatorLightningRod.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorLightningRod::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;

    if (!activation.character.isActor(result, true)) {
        return result;
    }

    activation.match.dungeon.accessWall(activation.room, activation.direction,
        [&](Cell& sourceCell) {
            Wall& sourceWall = activation.room.getWall(activation.direction);

            auto& inventory = activation.player.inventory;

            switch (sourceWall.door) {
                case DOOR_LIGHTNING_ROD_AWAKENED:
                    // give cube from rod
                    if (inventory.giveItem(ITEM_CUBE_AWAKENED, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_LIGHTNING_ROD_EMPTY;
                        result = CODE_SUCCESS;
                    }
                    return;
                case DOOR_LIGHTNING_ROD_DORMANT:
                    // give cube from rod
                    if (inventory.giveItem(ITEM_CUBE_DORMANT, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_LIGHTNING_ROD_EMPTY;
                        result = CODE_SUCCESS;
                    }
                    return;
                case DOOR_LIGHTNING_ROD_EMPTY:
                    // take cube for rod
                    if (inventory.takeItem(ITEM_CUBE_DORMANT, result) && activation.character.takeAction(result)) {
                        sourceWall.door = DOOR_LIGHTNING_ROD_AWAKENED;
                        result = CODE_SUCCESS;
                    }
                    return;
                default:
                    result = CODE_LIGHTNING_ROD_ACTIVATION_ON_NON_LIGHTNING_ROD;
                    return;
            }
        }
    );

    return result;
} 