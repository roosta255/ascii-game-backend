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

bool Wall::activateLock(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& codeset, MatchController& controller, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!accessDoor(codeset.error, [&](const DoorFlyweight& door) {
        if (door.isLockActionable) {
            if (!door.lockActivator.accessConst([&](const iActivator& activatorIntf) {
                Activation activation(player, character, room, Pointer<Character>::empty(), direction, match, codeset, controller, time);
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            })) {
                codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
                codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
                codeset.setTable(CODE_ACTIVATION_WALL_TYPE, this->door);
            }
        } else {
            codeset.addError(CODE_LOCK_ACTIVATOR_NOT_CONFIGURED);
            codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
            codeset.setTable(CODE_ACTIVATION_WALL_TYPE, this->door);
        }
    }));
    return isSuccess;
}

bool Wall::activateDoor(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& codeset, MatchController& controller, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!accessDoor(codeset.error, [&](const DoorFlyweight& door) {
        if (door.isDoorActionable) {
            if (!door.doorActivator.accessConst([&](const iActivator& activatorIntf) {
                Activation activation(player, character, room, Pointer<Character>::empty(), direction, match, codeset, controller, time);
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            })) {
                codeset.addError(CODE_DOOR_MISSING_ACTIVATOR);
                codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
                codeset.setTable(CODE_ACTIVATION_WALL_TYPE, this->door);
            }
        } else {
            codeset.addError(CODE_DOOR_ACTIVATOR_NOT_CONFIGURED);
            codeset.setTable(CODE_ACTIVATION_ROOM_ID, room.roomId);
            codeset.setTable(CODE_ACTIVATION_WALL_TYPE, this->door);
        }
    }));
    return isSuccess;
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
