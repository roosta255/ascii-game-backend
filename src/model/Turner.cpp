#include "Turner.hpp"
#include "Match.hpp"
#include "TurnEnum.hpp"
#include <json/json.h>

CodeEnum Turner::startGame() {
    switch (state) {
        case TURN_PREGAME_TITAN_ONLY:
            state = TURN_TITAN_ONLY;
            turn = 1;
            return CODE_SUCCESS;
        case TURN_PREGAME_BUILDER_ONLY:
            state = TURN_BUILDER_ONLY;
            turn = 1;
            return CODE_SUCCESS;
        case TURN_PREGAME_BOTH:
            state = TURN_BUILDER;
            turn = 1;
            return CODE_SUCCESS;
        default:
            return CODE_INVALID_TURN_STATE;
    }
}

CodeEnum Turner::addBuilder() {
    switch (state) {
        case TURN_PREGAME_BOTH:
            return CODE_SUCCESS;
        case TURN_PREGAME_TITAN_ONLY:
            state = TURN_PREGAME_BOTH;
            return CODE_SUCCESS;
        case TURN_PREGAME_BUILDER_ONLY:
            state = TURN_PREGAME_BOTH;
            return CODE_SUCCESS;
        case TURN_TITAN_ONLY:
            state = TURN_TITAN;
            return CODE_SUCCESS;
        case TURN_BUILDER_ONLY:
            state = TURN_BUILDER;
            return CODE_SUCCESS;
        default:
            return CODE_INVALID_TURN_STATE;
    }
}

CodeEnum Turner::addTitan() {
    switch (state) {
        case TURN_PREGAME_BOTH:
            return CODE_SUCCESS;
        case TURN_PREGAME_TITAN_ONLY:
            return CODE_SUCCESS;
        case TURN_PREGAME_BUILDER_ONLY:
            state = TURN_PREGAME_BOTH;
            return CODE_SUCCESS;
        case TURN_TITAN_ONLY:
            state = TURN_GAME_ABORTED;
            return CODE_SUCCESS;
        case TURN_BUILDER_ONLY:
            state = TURN_BUILDER;
            return CODE_SUCCESS;
        default:
            return CODE_INVALID_TURN_STATE;
    }
}

CodeEnum Turner::removeBuilder(const std::string& leaver, Match& match) {
    // if the game still has builders, don't change the state
    for (auto& builder : match.builders) {
        if (builder.player.account.isEmpty()) {
            continue;
        }
        if (builder.player.account != leaver) {
            return CODE_SUCCESS;
        }
    }

    // if the game has no builders, either titan only or aborted
    switch (state) {
        case TURN_PREGAME_BOTH:
            state = TURN_PREGAME_TITAN_ONLY;
            return CODE_SUCCESS;
        case TURN_PREGAME_TITAN_ONLY:
            return CODE_SUCCESS;
        case TURN_PREGAME_BUILDER_ONLY:
            state = TURN_GAME_ABORTED;
            return CODE_SUCCESS;
        case TURN_TITAN_ONLY:
            return CODE_SUCCESS;
        case TURN_BUILDER_ONLY:
            state = TURN_POSTGAME_BUILDERS_LEFT;
            return CODE_SUCCESS;
        default:
            return CODE_INVALID_TURN_STATE;
    }
}

bool Turner::isStarted() const {
    switch (state) {
        case TURN_PREGAME_BOTH:
        case TURN_PREGAME_TITAN_ONLY:
        case TURN_PREGAME_BUILDER_ONLY:
            return false;
        default:
            return true;      
    }
}

bool Turner::isCompleted() const {
    switch (state) {
        case TURN_POSTGAME_BUILDERS_LEFT:
        case TURN_POSTGAME_TITANS_LEFT:
        case TURN_GAME_ABORTED:
            return true;
    }
    return false;
}

CodeEnum Turner::removeTitan() {
    switch (state) {
        case TURN_PREGAME_BOTH:
            state = TURN_PREGAME_BUILDER_ONLY;
            return CODE_SUCCESS;
        case TURN_PREGAME_TITAN_ONLY:
            state = TURN_GAME_ABORTED;
            return CODE_SUCCESS;
        case TURN_PREGAME_BUILDER_ONLY:
            return CODE_SUCCESS;
        case TURN_TITAN_ONLY:
            state = TURN_POSTGAME_TITANS_LEFT;
            return CODE_SUCCESS;
        case TURN_BUILDER_ONLY:
            return CODE_SUCCESS;
        default:
            return CODE_INVALID_TURN_STATE;
    }
}

CodeEnum Turner::endBuilderTurn(Match& match) {
    const auto& endTurnProcess = [&](TurnEnum newState) {
        for (auto& builder : match.builders) {
            builder.endTurn(match);
        }
        turn++;
        state = newState;
        match.titan.startTurn(match);
    };
    switch (state) {
        case TURN_BUILDER:
            endTurnProcess(TURN_TITAN);
            return CODE_SUCCESS;
        case TURN_BUILDER_ONLY:
            endTurnProcess(TURN_BUILDER_ONLY);
            match.titan.endTurn(match);
            turn++;
            for (auto& builder : match.builders) {
                builder.startTurn(match);
            }
            return CODE_SUCCESS;
        default:
            return CODE_BUILDER_TURN_NOT_STARTED;
    }
}

CodeEnum Turner::endTitanTurn(Match& match) {
    const auto& endTurnProcess = [&](TurnEnum newState) {
        match.titan.endTurn(match);
        turn++;
        state = newState;
        for (auto& builder : match.builders) {
            builder.startTurn(match);
        }
    };
    switch (state) {
        case TURN_TITAN:
            endTurnProcess(TURN_BUILDER);
            return CODE_SUCCESS;
        case TURN_TITAN_ONLY:
            endTurnProcess(TURN_TITAN_ONLY);
            for (auto& builder : match.builders) {
                builder.endTurn(match);
            }
            turn++;
            match.titan.startTurn(match);
            return CODE_SUCCESS;
        default:
            return CODE_TITAN_TURN_NOT_STARTED;
    }
}

bool Turner::isTitansTurn() const {
    switch (state) {
        case TURN_TITAN:
        case TURN_TITAN_ONLY:
            return true;
    }
    return false;
}

bool Turner::isBuildersTurn() const {
    switch (state) {
        case TURN_BUILDER:
        case TURN_BUILDER_ONLY:
            return true;
    }
    return false;
}

void Turner::toJson(Json::Value& out) const {
    out["turn"] = turn;
    out["state"] = turn_to_text(state);
}

bool Turner::fromJson(const Json::Value& in) {
    if (!in.isMember("turn") || !in.isMember("state")) {
        return false;
    }
    
    turn = in["turn"].asInt();
    return text_to_turn(in["state"].asString(), state);
} 