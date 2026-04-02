#include "ActivatorAlterTraitAffliction.hpp"
#include "Character.hpp"
#include "Codeset.hpp"
#include "LoggedEvent.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

static void applySpec(Character& character, const AlterTraitAfflictionSpec& spec) {
    character.traitsAfflicted |= spec.set;
    character.traitsAfflicted = character.traitsAfflicted - spec.clear;
    character.traitsAfflicted.setIndexOff((size_t)TRAIT_NIL);
}

bool ActivatorAlterTraitAffliction::activate(Activation& activation) const {
    bool isSuccess = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& codeset = req.codeset;
        auto& subject = activation.character;

        if (config.actor.set.isAny() || config.actor.clear.isAny()) {
            applySpec(subject, config.actor);
            controller.updateTraits(subject);
            for (int i = 0; i < TRAIT_COUNT; i++) {
                if (config.actor.set[i].orElse(false)) {
                    const auto event = LoggedEvent{
                        EVENT_SET_TRAIT,
                        { EventComponentKind::ROLE, (int)subject.role },
                        { EventComponentKind::TRAIT, i },
                        { EventComponentKind::ROLE, (int)subject.role },
                        -1
                    };
                    controller.addLoggedEvent(activation, req.room.roomId, event);
                    controller.addRequestLoggedEvent(activation, event);
                }
            }
        }

        if (config.target.set.isAny() || config.target.clear.isAny()) {
            bool applied = false;
            activation.targetCharacter().access([&](Character& target) {
                applySpec(target, config.target);
                controller.updateTraits(target);
                for (int i = 0; i < TRAIT_COUNT; i++) {
                    if (config.target.set[i].orElse(false)) {
                        const auto event = LoggedEvent{
                            EVENT_SET_TRAIT,
                            { EventComponentKind::ROLE, (int)subject.role },
                            { EventComponentKind::TRAIT, i },
                            { EventComponentKind::ROLE, (int)target.role },
                            -1
                        };
                        controller.addLoggedEvent(activation, req.room.roomId, event);
                        controller.addRequestLoggedEvent(activation, event);
                    }
                }
                applied = true;
            });

            if (!applied && config.target.set.isAny()) {
                codeset.addFailure(true, CODE_ALTER_TRAIT_AFFLICTION_TARGET_NOT_CHARACTER);
                controller.addRequestLoggedEvent(activation, LoggedEvent{
                    EVENT_FAILS_BEING_CHARACTER,
                    { EventComponentKind::ROLE, (int)subject.role },
                    {}, {}, -1
                });
                return;
            }
        }

        isSuccess = true;
    });
    return isSuccess;
}
