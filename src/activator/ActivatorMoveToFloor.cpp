#include "ActivatorMoveToFloor.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToFloor::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& match = activation.match;
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;

    int floorId;
    if (codeset.addFailure(!activation.floorId.copy(floorId), CODE_ACTIVATION_FLOOR_ID_NOT_SPECIFIED)) {
        return false;
    }

    // Check for occupied target cell
    int conflictingCharacterId;
    if (codeset.addFailure(controller.isFloorOccupied(room.roomId, subject.location.channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
        return false;
    }

    // take move, first mutation so no going back
    if (codeset.addFailure(!subject.takeMove(codeset.error))) {
        return false;
    }

    const auto newLocation = Location::makeFloor(room.roomId, subject.location.channel, floorId);

    Location oldLocation;
    codeset.addFailure(!controller.updateCharacterLocation(subject, newLocation, oldLocation), CODE_UPDATE_CHARACTER_LOCATION_FAILED);

    if (!activation.isSkippingAnimations) {
        const auto keyframe = Keyframe::buildWalking(activation.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
        if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes), keyframe)) {
            codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
        }
    }

    return true;
}