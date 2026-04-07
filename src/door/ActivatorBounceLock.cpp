#include "ActivatorBounceLock.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorBounceLock::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        if (req.isSkippingAnimations) { result = true; return; }

        auto& subject = activation.character;
        auto& room = req.room;

        Cardinal direction;
        if (req.codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) return;

        if (subject.location.type == LOCATION_FLOOR) {
            Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(req.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, subject.location.data, direction));
        } else if (subject.location.type == LOCATION_DOOR || subject.location.type == LOCATION_DOOR_SHARED) {
            Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(req.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, (Cardinal)subject.location.data, direction));
        }

        result = true;
    });
    return result;
}
