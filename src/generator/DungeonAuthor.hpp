#pragma once

#include "Array.hpp"
#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "DungeonMutator.hpp"
#include <functional>
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

    bool setup4x1Room(const int4&);
    bool setup2x5Room(const int4&);
    bool setup3x3Room(const int4&);
    bool setupDoorway(const int4&, const Cardinal);
    bool setupJailer(const int4&, const Cardinal, const bool);
    bool setupKeeper(const int4&, const Cardinal, const bool);
    bool setupLadderUp(const int4&, const Cardinal);
    bool setupLightningRodRoom(const int4&, const bool isCubed, const bool isAwakened);
    bool setupPoleUp(const int4&, const Cardinal);
    bool setupPowerGeneratorRoom(const int4&);
    bool setupShifter(const int4&, const Cardinal, const bool);
    bool setupTimeGateRoomToFuture(const int4&, const bool isCubed, const bool isAwakened);
    bool setupTogglerBlue(const int4&, const Cardinal);
    bool setupTogglerOrange(const int4&, const Cardinal);
    bool setupTogglerSwitch(const int4&, int& outCharacterId, int& outFloorId);

    bool setupHorizontalWalls (std::initializer_list<int> row, int y, int z);
    bool setupVerticalWalls (std::initializer_list<int> row, int y, int z);
};
