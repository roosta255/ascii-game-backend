#include "ActivatorBounceLock.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "Keyframe.hpp"
#include "MatchController.hpp"
#include "Room.hpp"

bool ActivatorBounceLock::activate(Activation& activation) const {
    if (activation.isSkippingAnimations) return true;

    auto& subject = activation.character;
    auto& room = activation.room;

    Cardinal direction;
    if (!activation.direction.copy(direction)) return true;

    if (subject.location.type == LOCATION_FLOOR) {
        Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, subject.location.data, direction));
    } else if (subject.location.type == LOCATION_DOOR) {
        Keyframe::insertKeyframe(subject.keyframes, Keyframe::buildBounceLock(activation.time, MatchController::BOUNCE_LOCK_ANIMATION_DURATION, room.roomId, (Cardinal)subject.location.data, direction));
    }

    return true;
}
