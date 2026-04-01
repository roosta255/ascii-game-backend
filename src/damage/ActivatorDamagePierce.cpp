#include "ActivatorDamagePierce.hpp"
#include "Activation.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamagePierce::activate(Activation& activation) const {
    activation.targetCharacter().access([&](Character& target) {
        activation.request.access([&](RequestContext& req) {
            const auto traitsComputed = req.controller.getTraitsComputed(target.characterId);
            if (traitsComputed.final[TRAIT_DAMAGE_WEAKNESS_PIERCE].orElse(false)) {
                target.damage += 1;
            }
        });
    });
    return true;
}
