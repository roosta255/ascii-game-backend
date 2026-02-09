#include "ActivatorTimeGate.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "Location.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Room.hpp"

bool ActivatorTimeGate::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& room = activation.room;
    const auto roomId = room.roomId;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;

    Cardinal direction;
    if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
        return false;
    }

    Wall& sourceWall = activation.room.getWall(direction);

    // Check for occupied target cell
    if (codeset.addFailure(!controller.validateDoorNotOccupied(room.roomId, CHANNEL_CORPOREAL, TIME_GATE_DIRECTION), CODE_OCCUPIED_TARGET_TIME_GATE_CELL)) {
        return false;
    }

    int delta = 0;
    switch (activation.room.type) {
        case ROOM_TIME_GATE_TO_FUTURE:
            delta = 1;
            break;
        case ROOM_TIME_GATE_TO_PAST:
            delta = -1;
            break;
        default:
            codeset.addError(CODE_TIME_GATE_ACTIVATED_BUT_ROOM_ISNT_TIME_GATE);
            return false;
    }

    bool isSuccess = false;
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
                    const auto newLocation = Location::makeDoor(room2.roomId, subject.location.channel, TIME_GATE_DIRECTION);
                    Location oldLocation;
                    controller.updateCharacterLocation(subject, newLocation, oldLocation);

                    // take subject move
                    if (controller.takeCharacterMove(subject)) {

                        // animate
                        if (!activation.isSkippingAnimations) {
                            const Keyframe keyframe = Keyframe::buildWalking(activation.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
                            if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
                                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_TIME_GATE);
                            }
                        }
                        // this is the end of the movement!!!
                        isSuccess = true;
                        return;
                    }
                });

                if (codeset.addFailure(!isDeltaValid, CODE_TIME_GATE_ACTIVATED_BUT_NEIGHBOR_DNE)) {
                    return false;
                }
            }
            return isSuccess;
        default:
            codeset.addError(CODE_TIME_GATE_ACTIVATED_BUT_WALL_MISSING_AWAKENED_CUBE);
            return false;
    }

    codeset.addUnreachableError(__LINE__);
    return false;
}