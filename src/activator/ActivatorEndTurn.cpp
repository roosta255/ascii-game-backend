#include "ActivatorEndTurn.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorEndTurn::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        // Advances turn state only; does not tick NPC conducts. This activator is
        // also used to simulate an ACTION_END_TURN candidate move against
        // speculative/copied Match state during pathfinding (permuteCharacterActions),
        // where ticking would run full conduct proposals against a hypothetical state.
        bool titanTurnEnded = false;
        result = !req.codeset.addFailure(!MatchController::advanceTurnState(req.match, req.player.account.toString(), req.codeset.error, titanTurnEnded));
    });
    return result;
}
