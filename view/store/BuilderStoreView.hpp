#pragma once

#include "adl_serializer.hpp"
#include "Builder.hpp"
#include "CharacterStoreView.hpp"
#include "PlayerStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct BuilderStoreView
{
    CharacterStoreView character;
    PlayerStoreView player;
 
    inline BuilderStoreView() = default;
 
    inline BuilderStoreView(const Builder& model)
    : player(model.player), character(model.character) {}
 
    inline operator Builder() const {
        return Builder{
            .player = this->player,
            .character = this->character
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuilderStoreView, character, player)
