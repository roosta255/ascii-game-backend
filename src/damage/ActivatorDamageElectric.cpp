#include "ActivatorDamageElectric.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamageElectric::activate(Activation& activation) const {
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
