#include "ActivatorPole.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "DoorEnum.hpp"

bool ActivatorPole::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& subject = activation.character;
        auto& room = req.room;
        const auto roomId = room.roomId;

        Cardinal direction;
        if (codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) {
            return;
        }

        if (!controller.validateDoorNotOccupied(roomId, CHANNEL_CORPOREAL, direction)) {
            return;
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
                return;
        }

        if (!controller.validateDoorNotOccupied(neighborRoomId, CHANNEL_CORPOREAL, direction)) {
            return;
        }

        bool isSuccess = false;
        const bool isNeighborValid = req.match.dungeon.rooms.access(neighborRoomId, [&](Room& room2) {
            Location oldLocation;
            controller.updateCharacterLocation(subject, newLocation, oldLocation);

            if (controller.takeCharacterMove(subject)) {
                if (!req.isSkippingAnimations) {
                    const Keyframe keyframe = Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
                    if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
                        codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_ACTIVATE_POLE);
                    }
                }
                isSuccess = true;
                return;
            }
        });

        codeset.addFailure(!isNeighborValid, CODE_POLE_MISSING_NEIGHBORING_ROOM);

        result = isSuccess;
    });
    return result;
}
