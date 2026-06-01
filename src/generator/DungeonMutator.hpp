#pragma once

#include "DoorEnum.hpp"
#include "int4.hpp"
#include "ItemEnum.hpp"
#include "LockEnum.hpp"
#include "Maybe.hpp"
#include "Rack.hpp"
#include "Room.hpp"
#include "RoleEnum.hpp"
#include "RoomEnum.hpp"
#include <functional>
#include <variant>
#include <vector>

class Codeset;
class Dungeon;
class MatchController;

class DungeonMutator {
public:
    using RoomRef = std::variant<int, int4>;

private:
    Dungeon& dungeon;
    Codeset& codeset;
    MatchController& controller;
    std::function<Maybe<int>(const int4&)> coordinateResolver;

    // internal functions
    Pointer<Room> getRoom(const int& roomId);
    bool setDoor(DoorEnum& source, DoorEnum next);
    Maybe<int> resolveRoom(const RoomRef& room);

public:
    bool isElevatorOverride = false;

    // structs
    struct ChestSpec {
        LockEnum lock = LOCK_NIL;
        RoleEnum critterRole = ROLE_EMPTY;
        Maybe<RoleEnum> containedRole = Maybe<RoleEnum>::empty();
        std::vector<ItemEnum> items;
    };

    struct ElevatorProperties {
        Array<Maybe<int>, 4> connectedRoomIds;
        bool isPaid;
    };

    // constructors
    DungeonMutator(MatchController& controller);

    void setCoordinateResolver(std::function<Maybe<int>(const int4&)> resolver);

    // functions
    bool setDoor(const RoomRef& room, Cardinal dir, DoorEnum type, const Maybe<int> setRoomId = Maybe<int>::empty());
    bool setRoom(const RoomRef& room, RoomEnum type);
    bool setSharedShaftAbove(const RoomRef& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB);
    bool setSharedDoor(const RoomRef& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB, const Maybe<int> setRoomId = Maybe<int>::empty());

    bool setupChest(const RoomRef& room, const ChestSpec&);
    bool setup2x5Room(const RoomRef& room);
    bool setup3x3Room(const RoomRef& room);
    bool setup4x1Room(const RoomRef& room);
    bool setupDoorway(const RoomRef& room, Cardinal dir);
    bool setupExitDoor(const RoomRef& room, Cardinal dir);
    bool setupElevatorColumn(const RoomRef& elevatorRoom, const Rack<ElevatorProperties>& elevatorPropertyList);
    bool setupElevatorLevel(const RoomRef& elevatorRoom, const ElevatorProperties& connectedRoomIds, const bool isElevatorPresent, const bool isExistingHigher, const bool isExistingLower, const bool isHigherPaid = false, const bool isLowerPaid = false);
    bool setupJailer(const RoomRef& room, Cardinal dir, bool isKeyed);
    bool setupKeeper(const RoomRef& room, Cardinal dir, bool isKeyed);
    bool setupLadderUp(const RoomRef& bottomRoom, Cardinal dir);
    bool setupLightningRodRoom(const RoomRef& room, bool isCubed, bool isAwakened);
    bool setupPoleUp(const RoomRef& bottomRoom, Cardinal dir);
    bool setupPowerGeneratorRoom(const RoomRef& room);
    bool setupCovenantDoor(const RoomRef& room, Cardinal dir);
    bool setupShifter(const RoomRef& room, Cardinal dir, bool isKeyed);
    bool setupTimeGateRoomToFuture(const RoomRef& presentRoom, bool isCubed, bool isAwakened);
    bool setupTogglerBlue(const RoomRef& room, Cardinal dir);
    bool setupTogglerOrange(const RoomRef& room, Cardinal dir);
    bool setupTogglerSwitch(const RoomRef& room, int& outCharacterId, int& outRoomId);
    bool setupTogglerSwitchBlue(const RoomRef& room, int& outCharacterId, int& outRoomId);
    bool setupTogglerSwitchOrange(const RoomRef& room, int& outCharacterId, int& outRoomId);
    bool setupSacramentForgiveness(const RoomRef& room, int& outCharacterId, int& outFloorId);
    bool setBuilderStartingRoomId(int builderIndex, const RoomRef& room);
};
