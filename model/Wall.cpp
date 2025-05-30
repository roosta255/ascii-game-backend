#include "Wall.hpp"
#include "DoorFlyweight.hpp"

#include <drogon/drogon.h>
#include <json/json.h>

bool Wall::accessDoor
(
    std::function<void(const DoorFlyweight&)> consumer
) const {
    return DoorFlyweight::getFlyweights().accessConst(door, [&](const DoorFlyweight& flyweight){
        consumer(flyweight);
    });
}

void Wall::toJson(Json::Value& out) const {
    // cell
    Json::Value cellJson;
    cell.toJson(cellJson);
    out["cell"] = cellJson;

    // door
    DoorFlyweight::getFlyweights().accessConst(door, [&](const DoorFlyweight& flyweight){
        out["door"] = flyweight.name;
    });
}

bool Wall::fromJson(const Json::Value& in) {
    // cell
    if (!cell.fromJson(in["cell"])) {
        LOG_ERROR << "Wall json couldnt parse cell field";
        return false;
    }

    // door
    if (!in.isMember("door")) {
        LOG_ERROR << "Wall json is missing door field";
        return false;
    }
    auto doorname = in["door"].asString();
    if (!DoorFlyweight::indexByString(doorname, door)) {
        LOG_ERROR << "Wall json couldnt parse door " << doorname;
        return false;
    }

    return true;
}
