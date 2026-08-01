#pragma once

#include "CodeEnum.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "TurnEnum.hpp"
#include <functional>

namespace Json {
    class Value;
}

class Match;

class Turner {
public:
    // Player management
    CodeEnum addBuilder();    // Add builder player
    CodeEnum addTitan();      // Add titan player
    CodeEnum removeBuilder(const std::string& leaver, Match& match); // Remove builder player
    CodeEnum removeTitan();   // Remove titan player
    CodeEnum startGame();

    // Pure turn-state transitions, no tick side effect. Decides the next TurnEnum
    // state (and, for builders, whether this also ended the titan's turn) by
    // interpreting `state`.
    CodeEnum advanceTitanTurnState(Match& match);
    CodeEnum advanceBuilderTurnState(Match& match, bool& outTitanTurnEnded);

    // Runs a full NPC turn: gives every dungeon character a fresh moves/actions
    // budget, invokes tickConducts (the NPC conduct tick), then closes out the turn.
    void runNpcTurn(Match& match, std::function<void()> tickConducts);

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
    bool isStarted() const;
    bool isCompleted() const;
    bool isTitansTurn() const;
    bool isBuildersTurn() const;

    int turn = 0;
    TurnEnum state = TURN_PREGAME_TITAN_ONLY;  // Start in pregame state
private:
};

HASH_MACRO_DECL(Turner)
