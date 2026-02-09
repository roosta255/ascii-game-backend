#include "ActivatorEndTurn.hpp"
#include "Codeset.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool ActivatorEndTurn::activate(Activation& activation) const {
    auto& controller = activation.controller;
    auto& codeset = activation.codeset;
    auto& match = activation.match;
    auto& player = activation.player;
    auto& inventory = player.inventory;
    auto& room = activation.room;
    auto& subject = activation.character;

    return !codeset.addFailure(!match.endTurn(player.account.toString(), codeset.error));
}
