#pragma once

#include "DoorEnum.hpp"
#include "Maybe.hpp"
#include "Rack.hpp"
#include "Room.hpp"
#include "RoomEnum.hpp"

class Codeset;
class Dungeon;
class MatchController;

class DungeonMutator {
private:
    Dungeon& dungeon;
    Codeset& codeset;
    MatchController& controller;
public:
    bool isElevatorOverride = false;
private:
    // internal functions
    Pointer<Room> getRoom(const int& roomId);
    bool setDoor(DoorEnum& source, DoorEnum next);

public:
    // structs
    struct ElevatorProperties {
        Array<Maybe<int>, 4> connectedRoomIds;
        bool isPaid;
    };

    // constructors
    DungeonMutator(MatchController& controller);

    // functions
    bool setDoor(const int& roomId, Cardinal dir, DoorEnum type, const Maybe<int> setRoomId = Maybe<int>::empty());
    bool setRoom(const int& roomId, RoomEnum type);
    bool setSharedShaftAbove(const int& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB);
    bool setSharedDoor(const int& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB, const Maybe<int> setRoomId = Maybe<int>::empty());

    bool setup2x5Room(const int& roomId);
    bool setup3x3Room(const int& roomId);
    bool setup4x1Room(const int& roomId);
    bool setupDoorway(const int& roomId, Cardinal dir);
    bool setupExitDoor(const int& roomId, Cardinal dir);
    bool setupElevatorColumn(const int& elevatorRoomId, const Rack<ElevatorProperties>& elevatorPropertyList);
    bool setupElevatorLevel(const int elevatorRoomId, const ElevatorProperties& connectedRoomIds, const bool isElevatorPresent, const bool isExistingHigher, const bool isExistingLower, const bool isHigherPaid = false, const bool isLowerPaid = false);
    bool setupJailer(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupKeeper(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupLadderUp(const int& bottomRoom, Cardinal dir);
    bool setupLightningRodRoom(const int& roomId, bool isCubed, bool isAwakened);
    bool setupPoleUp(const int& bottomRoom, Cardinal dir);
    bool setupPowerGeneratorRoom(const int& roomId);
    bool setupCovenantDoor(const int& roomId, Cardinal dir);
    bool setupShifter(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupTimeGateRoomToFuture(const int& presentRoom, bool isCubed, bool isAwakened);
    bool setupTogglerBlue(const int& roomId, Cardinal dir);
    bool setupTogglerOrange(const int& roomId, Cardinal dir);
    bool setupTogglerSwitch(const int& roomId, int& outCharacterId, int& outRoomId);
    bool setupTogglerSwitchBlue(const int& roomId, int& outCharacterId, int& outRoomId);
    bool setupTogglerSwitchOrange(const int& roomId, int& outCharacterId, int& outRoomId);
    bool setupSacramentForgiveness(const int& roomId, int& outCharacterId, int& outFloorId);
};
