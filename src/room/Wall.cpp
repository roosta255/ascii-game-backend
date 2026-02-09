#include "Codeset.hpp"
#include "Wall.hpp"
#include "DoorFlyweight.hpp"
#include "iActivator.hpp"
#include "MatchController.hpp"

bool Wall::accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)> consumer) const {
    const bool isDoorFlyweightAccessible = DoorFlyweight::getFlyweights().accessConst(door, [&](const DoorFlyweight& flyweight){
        consumer(flyweight);
    });
    if (!isDoorFlyweightAccessible) error = CODE_INACCESSIBLE_DOOR_FLYWEIGHT;
    return isDoorFlyweightAccessible;
}

bool Wall::readIsSharedDoorway(CodeEnum& error, bool& isSharedDoorway) const {
    bool isSuccess = false;
    accessDoor(error, [&](const DoorFlyweight& flyweight){
        isSharedDoorway = flyweight.isSharedDoorway;
        isSuccess = true;
    });
    return isSuccess;
}

bool Wall::isWalkable(CodeEnum& error) const {
    bool result = false;
    accessDoor(error, [&](const DoorFlyweight& flyweight){
        if (flyweight.blocking) error = CODE_BLOCKING_DOOR_TYPE;
        else result = true;
    });
    return result;
}

std::ostream& operator<<(std::ostream& os, const Wall& rhs) {
    
}
