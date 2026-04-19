#include "ActivatorDamageCrush.hpp"
#include "ActivationContext.hpp"
#include "MatchController.hpp"

bool ActivatorDamageCrush::activate(ActivationContext& activation) const {
    activation.targetCharacter().access([&](Character& target) {
        activation.request.access([&](RequestContext& req) {
            req.controller.breakArmorItem(target);
        });
    });
    return true;
}
