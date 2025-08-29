#pragma once

#include "adl_serializer.hpp"
#include "Wall.hpp"
#include "CellStoreView.hpp"
#include "DoorFlyweight.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct WallStoreView
{
    // if (params.isSpriting()) {
    //     CodeEnum error;
    //     accessDoor(error, [&](const DoorFlyweight& flyweight){ out["isDoorway"] = flyweight.isDoorway; });
    // }
    std::string door = "UNPARSED_DOOR";
    CellStoreView cell;
 
    inline WallStoreView() = default;
 
    inline WallStoreView(const Wall& model)
    : cell(model.cell)
    {
        DoorFlyweight::getFlyweights().accessConst(model.door, [&](const DoorFlyweight& flyweight) {
            this->door = flyweight.name;
        });
    }
 
    inline operator Wall() const 
    {
        Wall model{
            .cell = this->cell
        };
        DoorFlyweight::indexByString(this->door, model.door);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WallStoreView, door, cell)
