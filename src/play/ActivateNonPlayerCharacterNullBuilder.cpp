#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "ActivationContext.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivateNonPlayerCharacterNullBuilder::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        result = (req.controller.endBuilderTurn() == CODE_SUCCESS);
    });
    return result;
}
