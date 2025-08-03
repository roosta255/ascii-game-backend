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
    Array<CellApiView, DUNGEON_ROOM_WIDTH * DUNGEON_ROOM_HEIGHT> floorCells;
    Array<WallApiView, 4> walls;
    bool isHidden = true;

    inline RoomApiView() = default;

    inline RoomApiView(const Room& model, const MatchApiParameters& params)
    : isHidden(params.isHidden(model.visibility))
    {
        if (isHidden) return;

        constexpr auto ROOM_CELL_WIDTH = 4;
        this->floorCells = model.floorCells.transform([&](const int i, const Cell& cell){
            const int x = i % ROOM_CELL_WIDTH;
            const int y = i / ROOM_CELL_WIDTH;
            return CellApiView(cell, params, x, y);
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
