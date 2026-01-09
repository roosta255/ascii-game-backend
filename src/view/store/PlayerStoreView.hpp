#pragma once

#include "adl_serializer.hpp"
#include "InventoryStoreView.hpp"
#include "Player.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct PlayerStoreView
{
    std::string account;
    InventoryStoreView inventory;

    inline PlayerStoreView() = default;
    inline PlayerStoreView(const Player& model)
    : account(model.account), inventory(model.inventory)
    {
    }
    inline operator Player() const {
        return Player{
            .account = this->account,
            .inventory = this->inventory
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerStoreView, account, inventory)
