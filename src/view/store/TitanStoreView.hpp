#pragma once

#include "adl_serializer.hpp"
#include "PlayerStoreView.hpp"
#include "Titan.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct TitanStoreView
{
    PlayerStoreView player;
 
    inline TitanStoreView() = default;
 
    inline TitanStoreView(const Titan& model)
    : player(model.player) 
    {

    }
 
    inline operator Titan() const {
        return Titan{
            .player = this->player
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TitanStoreView, player)
