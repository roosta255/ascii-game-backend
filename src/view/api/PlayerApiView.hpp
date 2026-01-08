#pragma once

#include "adl_serializer.hpp"
#include "InventoryView.hpp"
#include "Player.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct PlayerApiView
{
    std::string account;
    InventoryView inventory;

    inline PlayerApiView() = default;
    inline PlayerApiView(const Player& model, const MatchApiParameters& params)
    : account(model.account), inventory(model.inventory)
    {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerApiView, account, inventory)
