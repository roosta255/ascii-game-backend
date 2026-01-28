#include "ActivatorTimeGate.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "iLayout.hpp"
#include "Location.hpp"
#include "Inventory.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

CodeEnum ActivatorTimeGate::activate(Activation& activation) const {
    // Check if character can use keys
    CodeEnum result = CODE_PREACTIVATE_IN_ACTIVATOR;
    const auto roomId = activation.getRoomId();
    Codeset& codeset = activation.codeset;
    auto& character = activation.character;
    MatchController controller(activation.match, activation.codeset);

    if (!character.isMovable(result, true)) {
        return result;
    }

    Wall& sourceWall = activation.room.getWall(activation.direction);

    // Check for occupied target cell
    int outCharacterId;
    if (activation.controller.isDoorOccupied(activation.getRoomId(), CHANNEL_CORPOREAL, TIME_GATE_DIRECTION, outCharacterId)) {
        return CODE_OCCUPIED_TARGET_TIME_GATE_CELL;
    }

    auto& inventory = activation.player.inventory;
    int delta = 0;
    switch (activation.room.type) {
        case ROOM_TIME_GATE_TO_FUTURE:
            delta = 1;
            break;
        case ROOM_TIME_GATE_TO_PAST:
            delta = -1;
            break;
        default:
            return CODE_TIME_GATE_ACTIVATED_BUT_ROOM_ISNT_TIME_GATE;
    }

    switch (sourceWall.door) {
        case DOOR_TIME_GATE_AWAKENED:
            {
                const bool isDeltaValid = activation.match.dungeon.rooms.access(activation.room.getDeltaTime(delta), [&](Room& room2) {                    
                    // first mutation, no going back
                    // update the time gates
                    sourceWall.door = DOOR_TIME_GATE_DORMANT;
                    auto& wall2 = room2.getWall(TIME_GATE_DIRECTION);
                    wall2.door = DOOR_TIME_GATE_DORMANT;
                    
                    // clean and update locations
                    const auto newLocation = Location::makeDoor(roomId, character.location.channel, TIME_GATE_DIRECTION);
                    Location oldLocation;
                    activation.controller.updateCharacterLocation(character, newLocation, oldLocation);

                    // take character move
                    if (character.takeMove(result)) {

                        // animate
                        const Keyframe keyframe = Keyframe::buildWalking(activation.time, MatchController::MOVE_ANIMATION_DURATION, oldLocation, newLocation, codeset);
                        if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes), keyframe)) {
                            codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_TIME_GATE);
                        }
                        // this is the end of the movement!!!
                        result = CODE_SUCCESS;
                        return;
                    }
                });

                if (!isDeltaValid) {
                    return CODE_TIME_GATE_ACTIVATED_BUT_NEIGHBOR_DNE;
                }
            }
            return result;
        default:
            return CODE_TIME_GATE_ACTIVATED_BUT_WALL_MISSING_AWAKENED_CUBE;
    }


    return result;
}