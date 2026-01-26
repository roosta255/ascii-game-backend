#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Cell.hpp"
#include "CellStoreView.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "Wall.hpp"
#include "WallStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct RoomStoreView
{
    int visibility = 0;
    Array<CellStoreView, Room::DUNGEON_ROOM_CELL_COUNT> floorCells;
    Array<WallStoreView, 4> walls;
    std::string type = "UNPARSED_ROOM";
    int anterior = -1, posterior = -1, above = -1, below = -1;

    inline RoomStoreView() = default;

    inline RoomStoreView(const Room& model)
    : floorCells(model.reserveFloorCells.convert<CellStoreView>())
    , walls(model.walls.convert<WallStoreView>())
    , visibility(model.visibility)
    , anterior(model.anterior)
    , posterior(model.posterior)
    , above(model.above)
    , below(model.below)
    {
        RoomFlyweight::getFlyweights().accessConst(model.type, [&](const RoomFlyweight& flyweight) {
            this->type = flyweight.name;
        });
    }

    inline operator Room() const {
        Room model{
            .visibility = this->visibility,
            .reserveFloorCells = this->floorCells.convert<Cell>(),
            .walls = this->walls.convert<Wall>(),
            .anterior = this->anterior,
            .posterior = this->posterior,
            .above = this->above,
            .below = this->below
        };
        RoomFlyweight::indexByString(this->type, model.type);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomStoreView, floorCells, walls, type, anterior, posterior, above, below)
