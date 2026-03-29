#include "ActivatorToggler.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool ActivatorToggler::activate(Activation& activation) const {
    if (activation.request->codeset.addFailure(activation.target.isEmpty(), CODE_TARGET_CHARACTER_MISSING)) {
        return false;
    }
    bool isSuccess = false;
    activation.target.access([&](Character& target) {
        if (activation.request->controller.takeCharacterAction(target)) {
            // TOGGLER_ORANGE switches to blue; TOGGLER (legacy) and TOGGLER_BLUE switch to orange
            const RoleEnum newRole = (activation.character.role == ROLE_TOGGLER_ORANGE) ? ROLE_TOGGLER_BLUE : ROLE_TOGGLER_ORANGE;
            const auto newAnimation = newRole == ROLE_TOGGLER_BLUE ? ANIMATION_TOGGLER_SWITCH_BLUE : ANIMATION_TOGGLER_SWITCH_ORANGE;

            for (Character& character : activation.request->match.dungeon.characters) {
                if (character.role == ROLE_TOGGLER || character.role == ROLE_TOGGLER_BLUE || character.role == ROLE_TOGGLER_ORANGE) {
                    const RoleEnum oldRole = character.role;
                    character.role = newRole;
                    if (!activation.request->isSkippingAnimations) {
                        Keyframe::insertKeyframe(
                            Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes),
                            Keyframe::buildTransition(activation.request->time, 800, character.location.roomId, newAnimation, oldRole, newRole)
                        );
                    }
                }
            }

            activation.request->match.dungeon.toggleDoors(activation.request->time, activation.request->isSkippingAnimations);
            isSuccess = true;
            activation.request->controller.appendEventLog(activation, LoggedEvent{
                EVENT_TOGGLE,
                { EventComponentKind::ROLE, (int)activation.character.role },
                {},
                { EventComponentKind::ROLE, (int)newRole },
                -1
            });
        }
    });
    return isSuccess;
}