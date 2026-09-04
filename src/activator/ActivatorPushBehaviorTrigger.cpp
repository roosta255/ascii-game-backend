#include "ActivatorPushBehaviorTrigger.hpp"
#include "Character.hpp"
#include "MatchController.hpp"

bool ActivatorPushBehaviorTrigger::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        int targetId = -1;
        activation.targetCharacter().access([&](Character& target) { targetId = target.characterId; });
        req.controller.pushTrigger(nullptr, activation.character.characterId, targetId, event);
        result = true;
    });
    return result;
}
