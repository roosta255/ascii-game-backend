#pragma once

#include "Array.hpp"
#include "Cell.hpp"
#include "CodeEnum.hpp"
#include "Cardinal.hpp"
#include "Pointer.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Wall.hpp"
#include <functional>

constexpr int DUNGEON_ROOM_WIDTH = 4;
constexpr int DUNGEON_ROOM_HEIGHT = 5;

class Match;

struct Room {
    int visibility = ~0x0;
    Array<Cell, DUNGEON_ROOM_WIDTH * DUNGEON_ROOM_HEIGHT> floorCells;
    Array<Wall, 4> walls;

    Wall& getWall(Cardinal);
    const Wall& getWall(Cardinal) const;
    int getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const;

    // Iterate over all characters in the room (both floor and wall cells)
    void forEachCharacter(Match& match, std::function<void(Character&)> consumer);
    bool containsCharacter(int offset) const;
    bool containsFloorCell(const Cell& cell, CodeEnum& error, int& index) const;
    Pointer<Cell> getCell(int offset, CodeEnum& error);
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
