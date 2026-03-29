#include "ActivatorDamageCrush.hpp"
#include "Activation.hpp"
#include "MatchController.hpp"

bool ActivatorDamageCrush::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        activation.request->controller.breakArmorItem(target);
    });
    return true;
}
