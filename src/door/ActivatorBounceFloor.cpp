#include "ActivatorBounceFloor.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "LoggedEvent.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorBounceFloor::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        if (req.isSkippingAnimations) { result = true; return; }

        auto& subject = activation.character;
        auto& codeset = req.codeset;
        auto& room = req.room;

        Character* targetPtr = nullptr;
        activation.targetCharacter().access([&](Character& t) { targetPtr = &t; });

        if (!targetPtr) {
            codeset.addFailure(true, CODE_TARGET_CHARACTER_MISSING);
            req.controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_MISSING_TARGET,
                { EventComponentKind::ROLE, (int)subject.role },
                {}, {}, -1
            });
            return;
        }

        Character& source  = isActorSource ? subject   : *targetPtr;
        const int targetFloorId = isActorSource ? targetPtr->location.data : subject.location.data;

        if (source.location.type == LOCATION_FLOOR) {
            Keyframe::insertKeyframe(source.keyframes, Keyframe::buildBounceFloor(req.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, source.location.data, targetFloorId));
        } else if (source.location.type == LOCATION_DOOR) {
            Keyframe::insertKeyframe(source.keyframes, Keyframe::buildBounceFloor(req.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, (Cardinal)source.location.data, targetFloorId));
        }

        result = true;
    });
    return result;
}
