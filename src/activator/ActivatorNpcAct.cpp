#include "ActivatorNpcAct.hpp"
#include "ActivationContext.hpp"
#include "MatchController.hpp"
#include "RequestContext.hpp"

bool ActivatorNpcAct::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& request) {
        request.controller.getConductByCharacterId(activation.character.characterId)
            .access([&](Conduct& conduct) {
                result = conduct.buildAndExecuteProposals(activation);
            });
    });
    return result;
}
