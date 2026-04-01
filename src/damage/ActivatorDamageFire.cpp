#include "ActivatorDamageFire.hpp"
#include "Activation.hpp"
#include "Character.hpp"

bool ActivatorDamageFire::activate(Activation& activation) const {
    activation.targetCharacter().access([&](Character& target) {
        target.damage += 1;
    });
    return true;
}
