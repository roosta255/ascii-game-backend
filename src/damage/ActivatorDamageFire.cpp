#include "ActivatorDamageFire.hpp"
#include "ActivationContext.hpp"
#include "Character.hpp"

bool ActivatorDamageFire::activate(ActivationContext& activation) const {
    activation.targetCharacter().access([&](Character& target) {
        target.damage += 1;
    });
    return true;
}
