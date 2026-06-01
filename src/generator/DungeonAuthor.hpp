#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "DungeonMutator.hpp"
#include <functional>
#include <vector>
#include "int4.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "Room.hpp"
#include "RoomEnum.hpp"
#include <unordered_map>

class Codeset;
class Dungeon;
class MatchController;
class iLayout;

class DungeonAuthor {
public:
    using RoomRef = DungeonMutator::RoomRef;

private:
    int4 size{1,1,1,1};
    const iLayout* layout;
    std::unordered_map<int4, Pointer<Room>> mapping;
    Dungeon& dungeon;
    Codeset& codeset;
    MatchController& controller;
    DungeonMutator mutator;

public:
    DungeonAuthor(MatchController& controller, const iLayout& layout);

    Maybe<int> getRoomId(const int4&);

    bool setupChest(const RoomRef&, const DungeonMutator::ChestSpec&);
    bool setup4x1Room(const RoomRef&);
    bool setup2x5Room(const RoomRef&);
    bool setup3x3Room(const RoomRef&);
    bool setupDoorway(const RoomRef&, const Cardinal);
    bool setupExitDoor(const RoomRef&, const Cardinal);
    bool setupJailer(const RoomRef&, const Cardinal, const bool);
    bool setupKeeper(const RoomRef&, const Cardinal, const bool);
    bool setupLadderUp(const RoomRef&, const Cardinal);
    bool setupLightningRodRoom(const RoomRef&, const bool isCubed, const bool isAwakened);
    bool setupPoleUp(const RoomRef&, const Cardinal);
    bool setupPowerGeneratorRoom(const RoomRef&);
    bool setupCovenantDoor(const RoomRef&, const Cardinal);
    bool setupShifter(const RoomRef&, const Cardinal, const bool);
    bool setupTimeGateRoomToFuture(const RoomRef&, const bool isCubed, const bool isAwakened);
    bool setupTogglerBlue(const RoomRef&, const Cardinal);
    bool setupTogglerOrange(const RoomRef&, const Cardinal);
    bool setupTogglerSwitch(const RoomRef&, int& outCharacterId, int& outFloorId);
    bool setupSacramentForgiveness(const RoomRef&, int& outCharacterId, int& outFloorId);

    bool setupBuilderStartingRoomId(int builderIndex, const RoomRef&);
    bool setupElevatorColumn(int elevatorRoomId, const std::vector<int>& columnRoomIds, int paidIndex);
    bool setupHorizontalWalls (std::vector<DoorEnum> row, int y, int z);
    bool setupVerticalWalls (std::vector<DoorEnum> row, int y, int z);
};
