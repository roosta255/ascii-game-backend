#pragma once

#include "adl_serializer.hpp"
#include "PlayerApiView.hpp"
#include "Titan.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct TitanApiView
{
    PlayerApiView player;
 
    inline TitanApiView() = default;
 
    inline TitanApiView(const Titan& model, const MatchApiParameters& params)
    : player(model.player, params) 
    {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TitanApiView, player)
