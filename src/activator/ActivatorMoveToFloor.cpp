#include "ActivatorMoveToFloor.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToFloor::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = req.room;
        auto& subject = activation.character;

        int floorId;
        if (codeset.addFailure(!req.floorId.copy(floorId), CODE_ACTIVATION_FLOOR_ID_NOT_SPECIFIED)) {
            return;
        }

        int conflictingCharacterId;
        if (codeset.addFailure(controller.isFloorOccupied(room.roomId, subject.location.channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
            return;
        }

        if (codeset.addFailure(!subject.takeMove(codeset.error))) {
            return;
        }

        const auto newLocation = Location::makeFloor(room.roomId, subject.location.channel, floorId);

        Location oldLocation;
        codeset.addFailure(!controller.updateCharacterLocation(subject, newLocation, oldLocation), CODE_UPDATE_CHARACTER_LOCATION_FAILED);

        if (!req.isSkippingAnimations) {
            auto rack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes);
            const auto start = Keyframe::getLatestMovementEnd(rack, req.time);
            const auto keyframe = Keyframe::buildWalking(start, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(rack, keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
            }
        }

        result = true;
    });
    return result;
}
