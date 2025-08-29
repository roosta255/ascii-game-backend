#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "CharacterStoreView.hpp"
#include "Dungeon.hpp"
#include "LayoutFlyweight.hpp"
#include "RoomStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct DungeonStoreView
{
    std::string layout = "UNPARSED_LAYOUT";
    bool isBlueOpen = false;

    Array<CharacterStoreView, Dungeon::MAX_CHARACTERS> characters;
    Array<RoomStoreView, DUNGEON_ROOM_COUNT> rooms;

    inline DungeonStoreView() = default;

    inline DungeonStoreView(const Dungeon& model)
    : isBlueOpen(model.isBlueOpen)
    , characters(model.characters.convert<CharacterStoreView>())
    , rooms(model.rooms.convert<RoomStoreView>())
    {
        LayoutFlyweight::getFlyweights().accessConst(model.layout, [&](const LayoutFlyweight& flyweight){
            this->layout = flyweight.name;
        });
    }

    inline operator Dungeon() const {
        Dungeon model{
            .rooms = this->rooms.convert<Room>(),
            .characters = this->characters.convert<Character>(),
            .isBlueOpen = this->isBlueOpen
        };
        LayoutFlyweight::indexByString(this->layout, model.layout);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DungeonStoreView, isBlueOpen, layout, characters, rooms)
