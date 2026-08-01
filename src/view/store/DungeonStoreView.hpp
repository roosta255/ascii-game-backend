#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "CharacterStoreView.hpp"
#include "ChestStoreView.hpp"
#include "ConductStoreView.hpp"
#include "Dungeon.hpp"
#include "ItemStoreView.hpp"
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
    Array<ChestStoreView, Dungeon::MAX_CHESTS> chests;
    Array<ConductStoreView, Dungeon::MAX_CONDUCTS> conducts;
    Array<ItemStoreView, Dungeon::MAX_ITEMS> items;

    inline DungeonStoreView() = default;

    inline DungeonStoreView(const Dungeon& model)
    : isBlueOpen(model.isBlueOpen)
    , characters(model.characters.convert<CharacterStoreView>())
    , rooms(model.rooms.convert<RoomStoreView>())
    , chests(model.chests.convert<ChestStoreView>())
    , conducts(model.conducts.convert<ConductStoreView>())
    , items(model.items.convert<ItemStoreView>())
    {
        LayoutFlyweight::getFlyweights().accessConst(model.layout, [&](const LayoutFlyweight& flyweight){
            this->layout = flyweight.name;
        });
    }

    inline operator Dungeon() const {
        Dungeon model{
            .rooms = this->rooms.convert<Room>(),
            .characters = this->characters.convert<Character>(),
            .chests = this->chests.convert<Chest>(),
            .conducts = this->conducts.convert<Conduct>(),
            .items = this->items.convert<Item>(),
            .isBlueOpen = this->isBlueOpen
        };
        LayoutFlyweight::indexByString(this->layout, model.layout);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const DungeonStoreView& v) {
    j["isBlueOpen"]  = v.isBlueOpen;
    j["layout"]      = v.layout;
    j["characters"]  = v.characters;
    j["rooms"]       = v.rooms;
    j["chests"]      = v.chests;
    j["conducts"]    = v.conducts;
    j["items"]       = v.items;
}

inline void from_json(const nlohmann::json& j, DungeonStoreView& v) {
    j.at("isBlueOpen").get_to(v.isBlueOpen);
    j.at("layout").get_to(v.layout);
    j.at("characters").get_to(v.characters);
    j.at("rooms").get_to(v.rooms);
    v.chests   = j.value("chests",   Array<ChestStoreView,   Dungeon::MAX_CHESTS>{});
    v.conducts = j.value("conducts", Array<ConductStoreView, Dungeon::MAX_CONDUCTS>{});
    v.items    = j.value("items",    Array<ItemStoreView,     Dungeon::MAX_ITEMS>{});
}
