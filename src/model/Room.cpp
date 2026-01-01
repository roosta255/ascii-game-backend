#include "int2.hpp"
#include "Room.hpp"
#include "Match.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>

Wall& Room::getWall (Cardinal dir) {
    // shouldn't be possible
    static Wall nil;
    Wall* result = &nil;
    walls.access(dir.getIndex(), [&](Wall& accessed){
        result = &accessed;
    });
    return *result;
}

const Wall& Room::getWall (Cardinal dir) const {
    // shouldn't be possible
    static Wall nil;
    const Wall* result = &nil;
    walls.accessConst(dir.getIndex(), [&](const Wall& accessed){
        result = &accessed;
    });
    return *result;
}

int Room::getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const {
    return this - &rooms.head();
}

Pointer<Cell> Room::getCell(int offset, CodeEnum& error) {
    Pointer<Cell> result = floorCells.getPointer(offset);
    if (result.isEmpty()) {
        error = CODE_INACCESSIBLE_ROOM_FLOOR_CELL_ID;
    }
    return result;
}

void Room::forEachCharacter(Match& match, std::function<void(Character&)> consumer) {
    // Iterate over floor cells
    for (Cell& cell : floorCells) {
        CodeEnum error;
        match.getCharacter(cell.offset, error).access([&](Character& character){
            consumer(character);
        });
    }

    // Iterate over wall cells
    for (Wall& wall : walls) {
        CodeEnum error;
        match.getCharacter(wall.cell.offset, error).access([&](Character& character){
            consumer(character);
        });
    }
}

bool Room::containsCharacter(int offset) const {
    for (const auto& cell: floorCells) {
        if (cell.offset == offset) {
            return true;
        }
    }
    for (const auto& wall: walls) {
        if (wall.cell.offset == offset) {
            return true;
        }
    }
    return false;
}

bool Room::containsFloorCell(const Cell& cell, CodeEnum& error, int& index, int2& coords) const {
    if (floorCells.containsAddress(cell, index)) {
        coords = int2{index % Room::DUNGEON_ROOM_WIDTH,  index / Room::DUNGEON_ROOM_WIDTH};
        return true;
    }
    error = CODE_INACCESSIBLE_ROOM_FLOOR_CELL_ID;
    return false;
}


/*
void Room::toJson(JsonParameters& params, Json::Value& out) const {
    // floor cells
    Json::Value floorCellsJson(Json::arrayValue);
    int i = 0;
    constexpr auto ROOM_CELL_WIDTH = 4;
    for (auto& cell: floorCells) {
        Json::Value j;
        cell.toJson(params, j);
        if (params.isCoordinating()) {
            j["x"] = i % ROOM_CELL_WIDTH;
            j["y"] = i / ROOM_CELL_WIDTH;
        }
        floorCellsJson.append(j);
        i++;
    }
    out["floorCells"] = floorCellsJson;

    // walls
    Json::Value wallsJson(Json::arrayValue);
    int dir = 0;
    for (const auto& wall: walls) {
        Json::Value wallJson;
        wall.toJson(params, wallJson);
        if (params.isClient()) {
            params.match.dungeon.accessWall(*this, dir, 
                [&](const Cell&, const Cell&, const Room& adj){
                    int roomIndex = -1;
                    if (params.match.dungeon.containsRoom(adj, roomIndex)) {
                        wallJson["adjacent"] = roomIndex;
                    }
                },
                [](const Cell&){});
        }
        wallsJson.append(wallJson);
        dir++;
    }
    out["walls"] = wallsJson;
}
*/