#include "ActivatorMoveToFloor.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToFloor::activate(Activation& activation) const {
    auto& controller = activation.request->controller;
    auto& codeset = activation.request->codeset;
    auto& match = activation.request->match;
    auto& player = activation.request->player;
    auto& inventory = player.inventory;
    auto& room = activation.request->room;
    auto& subject = activation.character;

    int floorId;
    if (codeset.addFailure(!activation.request->floorId.copy(floorId), CODE_ACTIVATION_FLOOR_ID_NOT_SPECIFIED)) {
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

    if (!activation.request->isSkippingAnimations) {
        auto rack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes);
        const auto start = Keyframe::getLatestMovementEnd(rack, activation.request->time);
        const auto keyframe = Keyframe::buildWalking(start, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
        if(!Keyframe::insertKeyframe(rack, keyframe)) {
            codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
        }
    }

    return true;
}
