#pragma once

#include "adl_serializer.hpp"
#include "Player.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct PlayerStoreView
{
    std::string account;
    int itemStartIndex = -1;

    inline PlayerStoreView() = default;
    inline PlayerStoreView(const Player& model)
    : account(model.account.toString()), itemStartIndex(model.itemStartIndex)
    {
    }
    inline operator Player() const {
        return Player{
            .account = this->account,
            .itemStartIndex = this->itemStartIndex
        };
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerStoreView, account, itemStartIndex)
