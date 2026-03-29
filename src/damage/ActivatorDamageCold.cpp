#include "ActivatorDamageCold.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamageCold::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        const auto traitsComputed = activation.request->controller.getTraitsComputed(target.characterId);
        if (traitsComputed.final[TRAIT_MOVEMENT_READY].orElse(false)) {
            activation.request->codeset.addFailure(
                !activation.request->controller.takeCharacterMove(target),
                CODE_DAMAGE_FAILED_TO_APPLY_COLD_TAKE_MOVE
            );
        }
    });
    return true;
}
