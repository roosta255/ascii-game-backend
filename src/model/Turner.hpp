#pragma once

#include "CodeEnum.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "TurnEnum.hpp"

namespace Json {
    class Value;
}

class Match;

class Turner {
public:
    CodeEnum endBuilderTurn(Match& match);
    CodeEnum endTitanTurn(Match& match);

    // Player management
    CodeEnum addBuilder();    // Add builder player
    CodeEnum addTitan();      // Add titan player
    CodeEnum removeBuilder(const std::string& leaver, Match& match); // Remove builder player
    CodeEnum removeTitan();   // Remove titan player
    CodeEnum startGame();

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
