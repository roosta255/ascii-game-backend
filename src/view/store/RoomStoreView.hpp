#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Cell.hpp"
#include "CellStoreView.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Wall.hpp"
#include "WallStoreView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct RoomStoreView
{
    int visibility = 0;
    Array<CellStoreView, Room::DUNGEON_ROOM_CELL_COUNT> floorCells;
    Array<WallStoreView, 4> walls;

    inline RoomStoreView() = default;

    inline RoomStoreView(const Room& model)
    : floorCells(model.floorCells.convert<CellStoreView>())
    , walls(model.walls.convert<WallStoreView>())
    , visibility(model.visibility)
    {}

    inline operator Room() const {
        return Room{
            .visibility = this->visibility,
            .floorCells = this->floorCells.convert<Cell>(),
            .walls = this->walls.convert<Wall>()
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoomStoreView, floorCells, walls)
