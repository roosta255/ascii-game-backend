#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "CharacterApiView.hpp"
#include "Dungeon.hpp"
#include "LayoutFlyweight.hpp"
#include "RoomApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct DungeonApiView
{
    std::string layout = "UNPARSED_LAYOUT";
    bool isBlueOpen = false;

    Array<CharacterApiView, Dungeon::MAX_CHARACTERS> characters;
    Array<RoomApiView, DUNGEON_ROOM_COUNT> rooms;

    inline DungeonApiView() = default;

    inline DungeonApiView(const Dungeon& model, const MatchApiParameters& params)
    : isBlueOpen(model.isBlueOpen)
    , characters(model.characters.transform([&](const Character& character){return CharacterApiView(character, params);}))
    , rooms(model.rooms.transform([&](const Room& room){return RoomApiView(room, params);}))
    {
        LayoutFlyweight::getFlyweights().accessConst(model.layout, [&](const LayoutFlyweight& flyweight){
            this->layout = flyweight.name;
        });
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DungeonApiView, isBlueOpen, layout, characters, rooms)
