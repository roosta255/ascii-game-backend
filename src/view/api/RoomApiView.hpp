#pragma once

#include "adl_serializer.hpp"
#include "Array.hpp"
#include "Cell.hpp"
#include "CellApiView.hpp"
#include "Dungeon.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "Wall.hpp"
#include "WallApiView.hpp"
#include <string>
#include <nlohmann/json.hpp>
#include <vector>

struct RoomApiView
{
    std::vector<CellApiView> floorCells;
    Array<WallApiView, 4> walls;
    bool isHidden = true;
    std::string type = "UNPARSED_ROOM";
    int width = 0, height = 0;

    inline RoomApiView() = default;

    inline RoomApiView(const Room& model, const MatchApiParameters& params)
    : isHidden(params.isHidden(model.visibility))
    {
        if (isHidden) return;

        this->floorCells = model.getUsedFloorCells().transform([&](const int i, const Cell& cell){
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
        RoomFlyweight::getFlyweights().accessConst(model.type, [&](const RoomFlyweight& flyweight) {
            this->type = flyweight.name;
            this->width = flyweight.width;
            this->height = flyweight.height;
        });
    }
};

inline void to_json(nlohmann::json& j, const RoomApiView& view) {
    if (view.isHidden) {
        j = {{"isHidden", true}};
        return;
    }
    j = {
        {"floorCells", view.floorCells},
        {"walls", view.walls},
        {"width", view.width},
        {"height", view.height},
        {"type", view.type}
    };
}

inline void from_json(const nlohmann::json& j, RoomApiView& view) {
    // TODO: log an error or leave empty
}
