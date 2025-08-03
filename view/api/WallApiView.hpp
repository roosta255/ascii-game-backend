#pragma once

#include "adl_serializer.hpp"
#include "Wall.hpp"
#include "CellApiView.hpp"
#include "Compass.hpp"
#include "DoorFlyweight.hpp"
#include "int2.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct WallApiView
{
    std::string door = "UNPARSED_DOOR";
    CellApiView cell;
    bool isDoorway = false;
    int adjacent = -1;

    inline WallApiView() = default;

    static constexpr auto XY_COORDS = Compass<int2>(int2{1,1}, int2{2,2}, int2{3,3}, int2{4,4});
 
    inline WallApiView(const Wall& model, const MatchApiParameters& params, const Room& room, const Cardinal& dir)
    : cell(model.cell, params, XY_COORDS[dir][0], XY_COORDS[dir][1])
    {
        DoorFlyweight::getFlyweights().accessConst(model.door, [&](const DoorFlyweight& flyweight) {
            this->door = flyweight.name;
            this->isDoorway = flyweight.isDoorway;
        });
        params.match.dungeon.accessWall(room, dir, 
            [&](const Cell&, const Cell&, const Room& adj){
                params.match.dungeon.containsRoom(adj, this->adjacent = -2);
            },
            [](const Cell&){});
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WallApiView, door, cell, isDoorway, adjacent)
