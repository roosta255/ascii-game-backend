#pragma once

#include "adl_serializer.hpp"
#include "Builder.hpp"
#include "CharacterApiView.hpp"
#include "PlayerApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct BuilderApiView
{
    CharacterApiView character;
    PlayerApiView player;
 
    inline BuilderApiView() = default;
 
    inline BuilderApiView(const Builder& model, const MatchApiParameters& params)
    : player(model.player, params), character(model.character, params) {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuilderApiView, character, player)
