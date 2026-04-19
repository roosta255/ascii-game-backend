#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "ActivationContext.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullBuilder::activate(ActivationContext& activation) const {
    CodeEnum result = CODE_UNKNOWN_ERROR;
    activation.request.access([&](RequestContext& req) {
        result = req.match.turner.endBuilderTurn(req.match);
    });
    return result;
}
