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
    int startingRoomId = 0;

    inline BuilderStoreView() = default;

    inline BuilderStoreView(const Builder& model)
    : player(model.player), character(model.character), startingRoomId(model.startingRoomId) {}

    inline operator Builder() const {
        return Builder{
            .player = this->player,
            .character = this->character,
            .startingRoomId = this->startingRoomId
        };
    }
};

inline void to_json(nlohmann::json& j, const BuilderStoreView& v) {
    j = {{"character", v.character}, {"player", v.player}, {"startingRoomId", v.startingRoomId}};
}

inline void from_json(const nlohmann::json& j, BuilderStoreView& v) {
    j.at("character").get_to(v.character);
    j.at("player").get_to(v.player);
    v.startingRoomId = j.value("startingRoomId", 0);
}
