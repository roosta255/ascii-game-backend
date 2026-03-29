#include "ActivateNonPlayerCharacterNullBuilder.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullBuilder::activate(Activation& activation) const {
    CodeEnum result = CODE_UNKNOWN_ERROR;
    activation.request.access([&](RequestContext& req) {
        result = req.match.turner.endBuilderTurn(req.match);
    });
    return result;
}
