#pragma once

#include "adl_serializer.hpp"
#include "Dungeon.hpp"
#include "InventoryApiView.hpp"
#include "Match.hpp"
#include "MatchApiParameters.hpp"
#include "Player.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct PlayerApiView
{
    std::string account;
    InventoryApiView inventory;

    inline PlayerApiView() = default;
    inline PlayerApiView(const Player& model, const MatchApiParameters& params, int inventorySize)
    : account(model.account.toString())
    {
        auto& dungeon = const_cast<Dungeon&>(params.match.dungeon);
        auto inv = model.getInventory(dungeon);
        this->inventory = InventoryApiView(inv, model.itemStartIndex < 0 ? 0 : model.itemStartIndex, inventorySize);
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerApiView, account, inventory)
