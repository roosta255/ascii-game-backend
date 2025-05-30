#pragma once

#include "Cell.hpp"
#include <functional>

class DoorFlyweight;

namespace Json {
    class Value;
}

struct Wall {
    int door;
    Cell cell;

    bool accessDoor(std::function<void(const DoorFlyweight&)>)const;

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};