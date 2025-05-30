#pragma once

#include "Array.hpp"
#include "Cell.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Wall.hpp"

constexpr int DUNGEON_ROOM_WIDTH = 4;
constexpr int DUNGEON_ROOM_HEIGHT = 5;

namespace Json {
    class Value;
}

struct Room {
    Array<Cell, DUNGEON_ROOM_WIDTH * DUNGEON_ROOM_HEIGHT> floorCells;
    Array<Wall, 4> walls;

    Wall& getWall(Cardinal);
    int getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};

// every grid a cell
// pros:
// * fast cell lookups
// * a doorway can have 1 character exist in both rooms at once
// cons:
// * huge memory footprint
// * huge networking footprint (*this can be mitigated by tons of team-based masking in the backend, and handle fog-of-war in the client)
// * slow room lookups

// every character a location
// memory: MAX_CHARACTERS * ROOM_INDEX * CELL_INDEX
// pros:
// * smol memory footprint
// * smol networking footprint
// cons:
// * characters can exist on the floor and in a door
// * characters should exist in across both rooms if they're in a door, so it means doors either need an additional adj query anyway (not hard)
// * slow room lookup
// * slow cell lookup
