#include "ActivatorPole.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorPole::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& subject = activation.character;
    auto& inventory = activation.player.inventory;
    auto& room = activation.room;
    const auto direction = activation.direction;
    const auto roomId = room.roomId;

    // Check for occupied target cell
    if (!controller.validateDoorNotOccupied(roomId, CHANNEL_CORPOREAL, direction)) {
        return false;
    }

    Location newLocation;
    int neighborRoomId = -1;
    switch (room.getWall(direction).door) {
        case DOOR_POLE_1_TOP:
            neighborRoomId = room.below;
            newLocation = Location::makeShaftBottom(room.below, subject.location.channel, direction);
            break;
        case DOOR_POLE_1_BOTTOM:
            neighborRoomId = room.above;
            newLocation = Location::makeShaftTop(room.above, subject.location.channel, direction);
            break;
        default:
            codeset.addError(CODE_POLE_NEEDS_POLE_DOOR_TYPE);
            return false;
    }

    if (!controller.validateDoorNotOccupied(neighborRoomId, CHANNEL_CORPOREAL, direction)) {
        return false;
    }

    bool isSuccess = false;
    const bool isNeighborValid = activation.match.dungeon.rooms.access(neighborRoomId, [&](Room& room2) {     
        auto& wall2 = room2.getWall(direction);               

        // clean and update locations
        Location oldLocation;
        activation.controller.updateCharacterLocation(subject, newLocation, oldLocation);

        // take character move
        if (controller.takeCharacterMove(subject)) {

            // animate
            const Keyframe keyframe = Keyframe::buildWalking(activation.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_POLE);
            }

            // this is the end of the movement!!!
            isSuccess = true;
            return;
        }
    });

    codeset.addFailure(!isNeighborValid, CODE_POLE_MISSING_NEIGHBORING_ROOM);
    
    return isSuccess;
}