#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "ActivationContext.hpp"
#include "Match.hpp"

bool ActivateNonPlayerCharacterNullBuilder::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        result = (req.match.turner.endBuilderTurn(req.match) == CODE_SUCCESS);
    });
    return result;
}
