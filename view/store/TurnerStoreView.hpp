#pragma once

#include "adl_serializer.hpp"
#include "Turner.hpp"
#include "TurnEnum.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct TurnerStoreView
{
    std::string state = "UNPARSED_STATE";
    int turn;
 
    inline TurnerStoreView() = default;
 
    inline TurnerStoreView(const Turner& model)
    : turn(model.turn), state(turn_to_text(model.state))
    {}
 
    inline operator Turner() const
    {
        Turner model{
            .turn = this->turn
        };
        text_to_turn(this->state, model.state);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TurnerStoreView, state, turn)