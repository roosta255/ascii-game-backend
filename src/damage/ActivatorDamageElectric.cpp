#include "ActivatorDamageElectric.hpp"
#include "ActivationContext.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamageElectric::activate(ActivationContext& activation) const {
    activation.targetCharacter().access([&](Character& target) {
        activation.request.access([&](RequestContext& req) {
            const auto traitsComputed = req.controller.getTraitsComputed(target.characterId);
            if (traitsComputed.final[TRAIT_ACTION_READY].orElse(false)) {
                req.codeset.addFailure(
                    !req.controller.takeCharacterAction(target),
                    CODE_DAMAGE_FAILED_TO_APPLY_ELECTRIC_TAKE_ACTION
                );
            }
        });
    });
    return true;
}
