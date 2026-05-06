#pragma once

#include "Cardinal.hpp"
#include "DoorEnum.hpp"
#include "iRemodel.hpp"
#include "int4.hpp"
#include "ItemEnum.hpp"
#include "RemodelAuthorBase.hpp"
#include "RemodelAuthorChestSeeder.hpp"
#include "RemodelAuthorRunGeneratorLoop.hpp"
#include "RemodelAuthorSetupBuilderRole.hpp"
#include "RoleEnum.hpp"
#include <functional>
#include <vector>

class DungeonAuthor;
struct Remodel;

struct RemodelAuthorSetup4x1Room : public RemodelAuthorBase {
    int4 coords;
    explicit RemodelAuthorSetup4x1Room(int4 coords) : coords(coords) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetup2x5Room : public RemodelAuthorBase {
    int4 coords;
    explicit RemodelAuthorSetup2x5Room(int4 coords) : coords(coords) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetup3x3Room : public RemodelAuthorBase {
    int4 coords;
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupDoorway : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupDoorway(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupExitDoor : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupExitDoor(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupJailer : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    bool isKeyed;
    RemodelAuthorSetupJailer(int4 coords, Cardinal dir, bool isKeyed) : coords(coords), dir(dir), isKeyed(isKeyed) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupKeeper : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    bool isKeyed;
    RemodelAuthorSetupKeeper(int4 coords, Cardinal dir, bool isKeyed) : coords(coords), dir(dir), isKeyed(isKeyed) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupLadderUp : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupLadderUp(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupLightningRodRoom : public RemodelAuthorBase {
    int4 coords;
    bool isCubed;
    bool isAwakened;
    RemodelAuthorSetupLightningRodRoom(int4 coords, bool isCubed, bool isAwakened) : coords(coords), isCubed(isCubed), isAwakened(isAwakened) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupPoleUp : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupPoleUp(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupPowerGeneratorRoom : public RemodelAuthorBase {
    int4 coords;
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupCovenantDoor : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupShifter : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    bool isKeyed;
    RemodelAuthorSetupShifter(int4 coords, Cardinal dir, bool isKeyed) : coords(coords), dir(dir), isKeyed(isKeyed) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupTimeGateRoomToFuture : public RemodelAuthorBase {
    int4 coords;
    bool isCubed;
    bool isAwakened;
    RemodelAuthorSetupTimeGateRoomToFuture(int4 coords, bool isCubed, bool isAwakened) : coords(coords), isCubed(isCubed), isAwakened(isAwakened) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupTogglerBlue : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupTogglerBlue(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupTogglerOrange : public RemodelAuthorBase {
    int4 coords;
    Cardinal dir;
    RemodelAuthorSetupTogglerOrange(int4 coords, Cardinal dir) : coords(coords), dir(dir) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

// outCharacterId and outFloorId are written on mutateMatch for downstream use.
struct RemodelAuthorSetupTogglerSwitch : public RemodelAuthorBase {
    int4 coords;
    mutable int outCharacterId = -1;
    mutable int outFloorId     = -1;
    explicit RemodelAuthorSetupTogglerSwitch(int4 coords) : coords(coords) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupSacramentForgiveness : public RemodelAuthorBase {
    int4 coords;
    mutable int outCharacterId = -1;
    mutable int outFloorId     = -1;
    explicit RemodelAuthorSetupSacramentForgiveness(int4 coords) : coords(coords) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupElevatorColumn : public RemodelAuthorBase {
    int elevatorRoomId;
    std::vector<int> columnRoomIds;
    int paidIndex;
    RemodelAuthorSetupElevatorColumn(int elevatorRoomId, std::vector<int> columnRoomIds, int paidIndex)
        : elevatorRoomId(elevatorRoomId), columnRoomIds(std::move(columnRoomIds)), paidIndex(paidIndex) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupHorizontalWalls : public RemodelAuthorBase {
    std::vector<DoorEnum> row;
    int y, z;
    RemodelAuthorSetupHorizontalWalls(std::vector<DoorEnum> row, int y, int z)
        : row(std::move(row)), y(y), z(z) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

struct RemodelAuthorSetupVerticalWalls : public RemodelAuthorBase {
    std::vector<DoorEnum> row;
    int y, z;
    RemodelAuthorSetupVerticalWalls(std::vector<DoorEnum> row, int y, int z)
        : row(std::move(row)), y(y), z(z) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};

// Generic callable: stores any DungeonAuthor method as a function/lambda.
struct RemodelAuthorApply : public RemodelAuthorBase {
    std::function<bool(DungeonAuthor&)> call;
    explicit RemodelAuthorApply(std::function<bool(DungeonAuthor&)> c) : call(std::move(c)) {}
    bool mutateMatch(Remodel&, const Match&, const PathfindingCounter&, std::function<bool(const Match&)>) const override;
};
