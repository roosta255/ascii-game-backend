#pragma once

#include "adl_serializer.hpp"
#include "Turner.hpp"
#include "TurnEnum.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct TurnerApiView
{
    std::string state = "UNPARSED_STATE";
    int turn;
 
    inline TurnerApiView() = default;
 
    inline TurnerApiView(const Turner& model, const MatchApiParameters& params)
    : turn(model.turn), state(turn_to_text(model.state))
    {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TurnerApiView, state, turn)