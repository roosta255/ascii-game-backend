#include "Room.hpp"
#include <drogon/drogon.h>
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

int Room::getOffset(const Array<Room, DUNGEON_ROOM_COUNT>& rooms)const {
    return this - &rooms.head();
}

void Room::toJson(Json::Value& out) const {
    // door cells
    Json::Value wallsJson(Json::arrayValue);
    for (const auto& t: walls) {
        Json::Value j;
        t.toJson(j);
        wallsJson.append(j);
    }
    out["walls"] = wallsJson;

    // floor cells
    Json::Value floorCellsJson(Json::arrayValue);
    for (const auto& cell: floorCells) {
        Json::Value j;
        cell.toJson(j);
        floorCellsJson.append(j);
    }
    out["floorCells"] = floorCellsJson;

}

bool Room::fromJson(const Json::Value& in) {
    // walls
    const auto& wallsJson = in["walls"];
    if (!wallsJson.isArray()) {
        LOG_ERROR << "Dungeon json couldnt parse walls field due to it not being an array";
        return false;
    }
    if (wallsJson.size() != walls.size()) {
        LOG_ERROR << "Dungeon json couldnt parse walls field due to it not having " << walls.size() << " elements";
        return false;
    }
    int i = 0;
    for (auto& t: walls) {
        if (!t.fromJson(wallsJson[i])) {
            LOG_ERROR << "Dungeon json couldnt walls["<< i <<"] field";
            return false;
        }
        i++;
    }

    // floorCells
    const auto& floorCellsJson = in["floorCells"];
    if (!floorCellsJson.isArray()) {
        LOG_ERROR << "Dungeon json couldnt parse floorCells field due to it not being an array";
        return false;
    }
    if (floorCellsJson.size() != floorCells.size()) {
        LOG_ERROR << "Dungeon json couldnt parse floorCells field due to it not having " << floorCells.size() << " elements";
        return false;
    }
    int j = 0;
    for (auto& cell: floorCells) {
        if (!cell.fromJson(floorCellsJson[j])) {
            LOG_ERROR << "Dungeon json couldnt floorCells["<< j <<"] field";
            return false;
        }
        j++;
    }

    return true;
}
