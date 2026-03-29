#include "ActivatorEndTurn.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorEndTurn::activate(Activation& activation) const {
    auto& controller = activation.request->controller;
    auto& codeset = activation.request->codeset;
    auto& match = activation.request->match;
    auto& player = activation.request->player;
    auto& inventory = player.inventory;
    auto& room = activation.request->room;
    auto& subject = activation.character;

    return !codeset.addFailure(!match.endTurn(player.account.toString(), codeset.error));
}
