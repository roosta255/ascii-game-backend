#include "ActivatorDamageBite.hpp"
#include "Activation.hpp"
#include "ActivatorCritterBite.hpp"

bool ActivatorDamageBite::activate(Activation& activation) const {
    static ActivatorCritterBite critterBite;
    critterBite.activate(activation);
    return true;
}
