#pragma once

#include "adl_serializer.hpp"
#include "Player.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct PlayerStoreView
{
    std::string account;

    inline PlayerStoreView() = default;
    inline PlayerStoreView(const Player& model)
    : account(model.account) 
    {
    }
    inline operator Player() const {
        return Player{
            .account = this->account
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerStoreView, account)
