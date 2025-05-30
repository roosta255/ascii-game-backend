#include "Dungeon.hpp"
#include "iLayout.hpp"
#include "int4_to_json.hpp"
#include "LayoutFlyweight.hpp"
#include <drogon/drogon.h>
#include <json/json.h>

bool Dungeon::accessLayout(std::function<void(const iLayout&)> consumer) {
    bool isAccessed = false;
    LayoutFlyweight::getFlyweights().accessConst(layout, [&](const LayoutFlyweight& flyweight){
        flyweight.layout.accessConst([&](const iLayout& layoutIntf){
            isAccessed = true;
            consumer(layoutIntf);
        });
    });
    return isAccessed;
}

bool Dungeon::findCharacter(
    Room& room,
    int searched,
    std::function<void(Cell&, const Cardinal, Room&, Cell&)> wallConsumer,
    std::function<void(int, int, Cell&)> floorConsumer)
{
    bool isFound = false;
    // Search floor cells
    for (int y = 0; y < DUNGEON_ROOM_HEIGHT; ++y) {
        for (int x = 0; x < DUNGEON_ROOM_WIDTH; ++x) {
            int i = y * DUNGEON_ROOM_WIDTH + x;
            room.floorCells.access(i, [&](Cell& cell) {
                if (cell.offset == searched) {
                    isFound = true;
                    floorConsumer(x, y, cell);
                }
            });
        }
    }

    // Search wall cells
    for (Cardinal dir: Cardinal::getAllCardinals()) {
        Wall& wall = room.getWall(dir);

        if (wall.cell.offset == searched) {
            isFound = true;
            this->accessLayout([&](const iLayout& layoutIntf){
                layoutIntf.getWallNeighbor(rooms, room, dir).access([&](Room& neighbor) {
                    Cell& otherCell = neighbor.getWall(dir.getFlip()).cell;
                    if (otherCell.offset == searched) {
                        wallConsumer(wall.cell, dir, neighbor, otherCell);
                    }
                });
            });
        }
    }

    return isFound;
}


void Dungeon::toJson(Json::Value& out) const {
    // rooms
    Json::Value roomsJson(Json::arrayValue);
    for (const auto& room: rooms) {
        Json::Value b;
        room.toJson(b);
        b["offset"] = room.getOffset(rooms);
        roomsJson.append(b);
    }
    out["rooms"] = roomsJson;

    // layout
    LayoutFlyweight::getFlyweights().accessConst(layout, [&](const LayoutFlyweight& flyweight){
        out["layout"] = flyweight.name;
    });
}

bool Dungeon::fromJson(const Json::Value& in) {
    // rooms
    const auto& roomsJson = in["rooms"];
    if (!roomsJson.isArray()) {
        LOG_ERROR << "Dungeon json couldnt parse rooms field due to it not being an array";
        return false;
    }
    if (roomsJson.size() != rooms.size()) {
        LOG_ERROR << "Dungeon json couldnt parse rooms field due to it not having " << rooms.size() << " elements";
        return false;
    }
    int i = 0;
    for (auto& room: rooms) {
        if (!room.fromJson(roomsJson[i])) {
            LOG_ERROR << "Dungeon json couldnt parse rooms["<< i <<"] field";
            return false;
        }
        i++;
    }

    // layout
    if (!in.isMember("layout")) {
        LOG_ERROR << "Dungeon json is missing layout field";
        return false;
    }
    auto layoutName = in["layout"].asString();
    if (!LayoutFlyweight::indexByString(layoutName, layout)) {
        LOG_ERROR << "Dungeon json couldnt parse layout " << layoutName;
        return false;
    }

    return true;
}
