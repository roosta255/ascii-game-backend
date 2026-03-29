#include "ActivatorEndTurn.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorEndTurn::activate(Activation& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        result = !req.codeset.addFailure(!req.match.endTurn(req.player.account.toString(), req.codeset.error));
    });
    return result;
}
