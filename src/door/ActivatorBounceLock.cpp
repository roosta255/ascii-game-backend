#include "ActivatorBounceLock.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "Keyframe.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorBounceLock::activate(Activation& activation) const {
    if (activation.request->isSkippingAnimations) return true;

    auto& subject = activation.character;
    auto& room = activation.request->room;

    Cardinal direction;
    if (activation.request->codeset.addFailure(!activation.direction.copy(direction), CODE_ACTIVATION_DIRECTION_NOT_SPECIFIED)) return false;

    if (subject.location.type == LOCATION_FLOOR) {
        Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.request->time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, subject.location.data, direction));
    } else if (subject.location.type == LOCATION_DOOR) {
        Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.request->time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, (Cardinal)subject.location.data, direction));
    }

    return true;
}
