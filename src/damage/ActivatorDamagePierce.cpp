#include "ActivatorDamagePierce.hpp"
#include "Activation.hpp"
#include "MatchController.hpp"
#include "TraitEnum.hpp"

bool ActivatorDamagePierce::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        const auto traitsComputed = activation.controller.getTraitsComputed(target.characterId);
        if (traitsComputed.final[TRAIT_DAMAGE_WEAKNESS_PIERCE].orElse(false)) {
            target.damage += 1;
        }
    });
    return true;
}
