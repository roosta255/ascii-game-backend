#include "ActivatorDamageCold.hpp"
#include "Activation.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Codeset.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamageCold::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        activation.request.access([&](RequestContext& req) {
            const auto traitsComputed = req.controller.getTraitsComputed(target.characterId);
            if (traitsComputed.final[TRAIT_MOVEMENT_READY].orElse(false)) {
                req.codeset.addFailure(
                    !req.controller.takeCharacterMove(target),
                    CODE_DAMAGE_FAILED_TO_APPLY_COLD_TAKE_MOVE
                );
            }
        });
    });
    return true;
}
