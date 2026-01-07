#pragma once

#include "adl_serializer.hpp"
#include "Builder.hpp"
#include "CharacterApiView.hpp"
#include "InventoryView.hpp"
#include "PlayerApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct BuilderApiView
{
    CharacterApiView character;
    PlayerApiView player;
    InventoryView inventory;
 
    inline BuilderApiView() = default;
 
    inline BuilderApiView(const Builder& model, const MatchApiParameters& params)
    : player(model.player, params), character(model.character, params), inventory(model.inventory) {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuilderApiView, character, player, inventory)
