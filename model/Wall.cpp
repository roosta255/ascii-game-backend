#include "Wall.hpp"
#include "DoorFlyweight.hpp"
#include "iActivator.hpp"
#include "JsonParameters.hpp"
#include <json/json.h>

bool Wall::isWalkable(CodeEnum& error) const {
    bool result = false;
    accessDoor(error, [&](const DoorFlyweight& flyweight){
        if (flyweight.blocking) error = CODE_BLOCKING_DOOR_TYPE;
        else result = true;
    });
    return result;
}

bool Wall::accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)> consumer) const {
    const bool isDoorFlyweightAccessible = DoorFlyweight::getFlyweights().accessConst(door, [&](const DoorFlyweight& flyweight){
        consumer(flyweight);
    });
    if (!isDoorFlyweightAccessible) error = CODE_INACCESSIBLE_DOOR_FLYWEIGHT;
    return isDoorFlyweightAccessible;
}

CodeEnum Wall::activate(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match) {
    CodeEnum result = CODE_PREACTIVATE_IN_WALL;
    accessDoor(result, [&](const DoorFlyweight& door) {
        if (!door.activator.accessConst([&](const iActivator& activatorIntf) {
            Activation activation(player, character, room, Pointer<Character>::empty(), direction, match);
            result = activatorIntf.activate(activation);
        })) {
            result = CODE_DOOR_MISSING_ACTIVATOR;
        }
    });
    return result;
}
