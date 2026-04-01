#include "ActivatorToggler.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "EventFlyweight.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleEnum.hpp"

bool ActivatorToggler::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        if (req.codeset.addFailure(activation.targetCharacter().isEmpty(), CODE_TARGET_CHARACTER_MISSING)) {
            req.controller.addRequestLoggedEvent(activation, LoggedEvent{
                EVENT_MISSING_TARGET,
                { EventComponentKind::ROLE, (int)activation.character.role },
                {}, {}, -1
            });
            return;
        }
        activation.targetCharacter().access([&](Character& target) {
            if (req.controller.takeCharacterAction(target)) {
                const RoleEnum newRole = (activation.character.role == ROLE_TOGGLER_ORANGE) ? ROLE_TOGGLER_BLUE : ROLE_TOGGLER_ORANGE;
                const auto newAnimation = newRole == ROLE_TOGGLER_BLUE ? ANIMATION_TOGGLER_SWITCH_BLUE : ANIMATION_TOGGLER_SWITCH_ORANGE;

                for (Character& character : req.match.dungeon.characters) {
                    if (character.role == ROLE_TOGGLER || character.role == ROLE_TOGGLER_BLUE || character.role == ROLE_TOGGLER_ORANGE) {
                        const RoleEnum oldRole = character.role;
                        character.role = newRole;
                        if (!req.isSkippingAnimations) {
                            Keyframe::insertKeyframe(
                                Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes),
                                Keyframe::buildTransition(req.time, 800, character.location.roomId, newAnimation, oldRole, newRole)
                            );
                        }
                    }
                }

                req.match.dungeon.toggleDoors(req.time, req.isSkippingAnimations);
                isSuccess = true;
                req.controller.addLoggedEvent(activation, req.room.roomId, LoggedEvent{
                    EVENT_TOGGLE,
                    { EventComponentKind::ROLE, (int)activation.character.role },
                    {},
                    { EventComponentKind::ROLE, (int)newRole },
                    -1
                });
            } else {
                req.controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_NO_ACTIONS,
                    { EventComponentKind::ROLE, (int)target.role },
                    {}, {}, -1
                });
            }
        });
    });
    return isSuccess;
}
