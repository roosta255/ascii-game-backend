#include "ActivatorDamageElectric.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamageElectric::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        const auto traitsComputed = activation.controller.getTraitsComputed(target.characterId);
        if (traitsComputed.final[TRAIT_ACTION_READY].orElse(false)) {
            activation.codeset.addFailure(
                !activation.controller.takeCharacterAction(target),
                CODE_DAMAGE_FAILED_TO_APPLY_ELECTRIC_TAKE_ACTION
            );
        }
    });
    return true;
}
