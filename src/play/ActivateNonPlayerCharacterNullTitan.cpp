#include "ActivateNonPlayerCharacterNullTitan.hpp"
#include "Activation.hpp"
#include "Match.hpp"

CodeEnum ActivateNonPlayerCharacterNullTitan::activate(Activation& activation) const {
    CodeEnum result = CODE_UNKNOWN_ERROR;
    activation.request.access([&](RequestContext& req) {
        result = req.match.turner.endTitanTurn(req.match);
    });
    return result;
}
