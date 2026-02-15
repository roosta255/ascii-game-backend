#pragma once

#include "DoorEnum.hpp"
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

    Pointer<Room> getRoom(const int& roomId);

public:
    DungeonMutator(MatchController& controller);

    bool setDoor(const int& roomId, Cardinal dir, DoorEnum type);
    bool setRoom(const int& roomId, RoomEnum type);
    bool setSharedShaftAbove(const int& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB);
    bool setSharedDoor(const int& roomA, Cardinal dir, DoorEnum doorA, DoorEnum doorB);

    bool setup2x5Room(const int& roomId);
    bool setup3x3Room(const int& roomId);
    bool setup4x1Room(const int& roomId);
    bool setupElevatorRoom(const int& elevatorRoomId, const Array<Maybe<int>, 4>& connectedRoomIds);
    bool setupDoorway(const int& roomId, Cardinal dir);
    bool setupJailer(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupKeeper(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupLadderUp(const int& bottomRoom, Cardinal dir);
    bool setupLightningRodRoom(const int& roomId, bool isCubed, bool isAwakened);
    bool setupPoleUp(const int& bottomRoom, Cardinal dir);
    bool setupPowerGeneratorRoom(const int& roomId);
    bool setupShifter(const int& roomId, Cardinal dir, bool isKeyed);
    bool setupTimeGateRoomToFuture(const int& presentRoom, bool isCubed, bool isAwakened);
    bool setupTogglerBlue(const int& roomId, Cardinal dir);
    bool setupTogglerOrange(const int& roomId, Cardinal dir);
    bool setupTogglerSwitch(const int& roomId, int& outCharacterId, int& outRoomId);
};
