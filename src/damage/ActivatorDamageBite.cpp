#include "ActivatorDamageBite.hpp"
#include "ActivationContext.hpp"
#include "ActivatorCritterBite.hpp"

bool ActivatorDamageBite::activate(ActivationContext& activation) const {
    static ActivatorCritterBite critterBite;
    critterBite.activate(activation);
    return true;
}
