#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Cell.hpp"
#include "CellApiView.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "Wall.hpp"
#include "WallApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct RoomApiView
{
    Array<CellApiView, Room::DUNGEON_ROOM_CELL_COUNT> floorCells;
    Array<WallApiView, 4> walls;
    bool isHidden = true;

    inline RoomApiView() = default;

    inline RoomApiView(const Room& model, const MatchApiParameters& params)
    : isHidden(params.isHidden(model.visibility))
    {
        if (isHidden) return;

        constexpr auto ROOM_CELL_WIDTH = 4;
        this->floorCells = model.floorCells.transform([&](const int i, const Cell& cell){
            int2 xy;
            int index;
            CodeEnum error; // shouldnt ever error because iterating through room's cells
            model.containsFloorCell(cell, error, index, xy);
            return CellApiView(cell, params, xy[0], xy[1]);
        });
        this->walls = model.walls.transform(
            [&](const int i, const Wall& wall){
                return WallApiView(wall, params, model, Cardinal(i));
            }
        );
    }
};

inline void to_json(nlohmann::json& j, const RoomApiView& view) {
    // default or debugging version, if you still want one
    if (view.isHidden) {
        j = {{"isHidden", true}};
        return;
    }
    j = {
        {"floorCells", view.floorCells},
        {"walls", view.walls}
    };
}

inline void from_json(const nlohmann::json& j, RoomApiView& view) {
    // TODO: log an error or leave empty
}
