#include "ActivatorDamageCrush.hpp"
#include "Activation.hpp"
#include "MatchController.hpp"

bool ActivatorDamageCrush::activate(Activation& activation) const {
    activation.target.access([&](Character& target) {
        activation.request.access([&](RequestContext& req) {
            req.controller.breakArmorItem(target);
        });
    });
    return true;
}
