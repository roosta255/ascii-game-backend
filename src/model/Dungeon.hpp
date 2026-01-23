#pragma once

#include <functional>
#include <json/json.h>

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "int4.hpp"
#include "Room.hpp"
#include "Character.hpp"
#include "Pointer.hpp"
#include "Cardinal.hpp"

namespace Json {
    class Value;
}

class iLayout;
class JsonParameters;
class LayoutFlyweight;
class Match;

struct Dungeon {
    static constexpr int MAX_CHARACTERS = 32;

    Array<Room, DUNGEON_ROOM_COUNT> rooms;
    Array<Character, MAX_CHARACTERS> characters;
    int layout = 0;
    bool isBlueOpen = false;  // When true, blue doors are open and orange doors are closed

    bool findCharacter(
        Room& source,
        int offset,
        std::function<void(Cell&, const Cardinal, Room&, Cell&)> wallConsumer,
        std::function<void(int, int, Cell&)> floorConsumer);

    bool accessLayout(CodeEnum& error, std::function<void(const iLayout&)>)const;
    bool accessLayoutFlyweight(CodeEnum& error, std::function<void(const LayoutFlyweight&)>)const;

    // Toggles all blue/orange doors in the dungeon based on isBlueOpen
    void toggleDoors();

    // Helper functions for room management
    bool containsRoom(const Room& room, int& roomId) const;
    Pointer<Room> getRoom(int roomId, CodeEnum&);
    Pointer<const Room> getRoom(int roomId, CodeEnum&) const;

    bool accessWall(
        Room& source,
        Cardinal dir,
        std::function<void(Cell&)> callback
    );

    bool accessWall(
        Room& source,
        Cardinal dir,
        std::function<void(Cell&, Cell&, Room&)> neighborCallback,
        std::function<void(Cell&)> noNeighborCallback
    );

    bool accessWall(
        const Room& source,
        Cardinal dir,
        std::function<void(const Cell&, const Cell&, const Room&)> neighborCallback,
        std::function<void(const Cell&)> noNeighborCallback
    ) const;

    void toJson(JsonParameters& params, Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};
