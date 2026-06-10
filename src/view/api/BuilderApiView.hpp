#pragma once

#include "adl_serializer.hpp"
#include "Builder.hpp"
#include "CharacterApiView.hpp"
#include "CodeEnum.hpp"
#include "PlayerApiView.hpp"
#include "RoleFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct BuilderApiView
{
    CharacterApiView character;
    PlayerApiView player;
 
    inline BuilderApiView() = default;
 
    inline BuilderApiView(const Builder& model, const MatchApiParameters& params) {
        int invSize = 0;
        CodeEnum roleError = CODE_UNKNOWN_ERROR;
        model.character.accessRole(roleError, [&](const RoleFlyweight& fw) { invSize = fw.inventorySize; });
        this->player = PlayerApiView(model.player, params, invSize);
        this->character = CharacterApiView(model.character, params);
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuilderApiView, character, player)
