#include "Codeset.hpp"
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

bool Wall::activateLock(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& codeset, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!accessDoor(codeset.error, [&](const DoorFlyweight& door) {
        if (door.isLockActionable) {
            if (!door.lockActivator.accessConst([&](const iActivator& activatorIntf) {
                Activation activation(player, character, room, Pointer<Character>::empty(), direction, match, codeset, time);
                isSuccess = codeset.addSuccessElseFailureIfCodedSuccess(activatorIntf.activate(activation));
            })) {
                codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
            }
        } else {
            codeset.addError(CODE_LOCK_ACTIVATOR_NOT_CONFIGURED);
        }
    }));
    return isSuccess;
}

bool Wall::activateDoor(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& codeset, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!accessDoor(codeset.error, [&](const DoorFlyweight& door) {
        if (door.isDoorActionable) {
            if (!door.doorActivator.accessConst([&](const iActivator& activatorIntf) {
                Activation activation(player, character, room, Pointer<Character>::empty(), direction, match, codeset, time);
                isSuccess = codeset.addSuccessElseFailureIfCodedSuccess(activatorIntf.activate(activation));
            })) {
                codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
            }
        } else {
            codeset.addError(CODE_DOOR_ACTIVATOR_NOT_CONFIGURED);
        }
    }));
    return isSuccess;
}
