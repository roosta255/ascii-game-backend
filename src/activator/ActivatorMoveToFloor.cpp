#include "ActivatorMoveToFloor.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorMoveToFloor::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& room = activation.room;
        auto& subject = activation.character;

        int floorId;
        if (codeset.addFailure(!req.floorId.copy(floorId), CODE_ACTIVATION_FLOOR_ID_NOT_SPECIFIED)) {
            return;
        }

        int conflictingCharacterId;
        if (codeset.addFailure(controller.isFloorOccupied(room.roomId, subject.location.channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
            RoleEnum occRole = ROLE_EMPTY;
            CodeEnum charError = CODE_UNKNOWN_ERROR;
            req.match.getCharacter(conflictingCharacterId, charError).access([&](Character& c) { occRole = c.role; });
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_OCCUPIED_FLOOR,
                { EventComponentKind::ROLE, (int)occRole },
                {}, {}, -1
            });
            return;
        }

        if (codeset.addFailure(!subject.takeMove(codeset.error))) {
            controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_NO_MOVES,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        const auto newLocation = Location::makeFloor(room.roomId, subject.location.channel, floorId);

        Location oldLocation;
        codeset.addFailure(!controller.updateCharacterLocation(subject, newLocation, oldLocation), CODE_UPDATE_CHARACTER_LOCATION_FAILED);

        if (!req.isSkippingAnimations) {
            auto rack = Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(subject.keyframes);
            const auto keyframe = Keyframe::buildWalking(req.time, MatchController::MOVE_ANIMATION_DURATION, room.roomId, oldLocation, newLocation, codeset);
            if(!Keyframe::insertKeyframe(rack, keyframe)) {
                codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
            }
        }

        result = true;
    });
    return result;
}
