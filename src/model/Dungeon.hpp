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

    bool accessLayout(CodeEnum& error, std::function<void(const iLayout&)>)const;
    bool accessLayoutFlyweight(CodeEnum& error, std::function<void(const LayoutFlyweight&)>)const;

    // Toggles all blue/orange doors in the dungeon based on isBlueOpen
    void toggleDoors();

    // Helper functions for room management
    bool containsRoom(const Room& room, int& roomId) const;
    Pointer<Room> getRoom(int roomId, CodeEnum&);
    Pointer<const Room> getRoom(int roomId, CodeEnum&) const;

    bool accessWallNeighbor(Room& source, Cardinal dir, std::function<void(Wall&, Room&, int)> consumer);
    bool accessWallNeighbor(const Room& source, Cardinal dir, std::function<void(const Wall&, const Room&, int)> consumer) const;

};
