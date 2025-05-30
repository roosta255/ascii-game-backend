#pragma once

#include <functional>
#include "Array.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "int4.hpp"
#include "Room.hpp"

namespace Json {
    class Value;
}

class iLayout;

struct Dungeon {

    Array<Room, DUNGEON_ROOM_COUNT> rooms;
    int layout = 0;

    bool findCharacter(
        Room& source,
        int offset,
        std::function<void(Cell&, const Cardinal, Room&, Cell&)> wallConsumer,
        std::function<void(int, int, Cell&)> floorConsumer);

    bool accessLayout(std::function<void(const iLayout&)>);

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};
