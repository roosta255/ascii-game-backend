#include "DungeonAuthor.hpp"
#include "iLayout.hpp"
#include "LayoutFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Remodel.hpp"
#include "RemodelAuthor.hpp"
#include <functional>

bool RemodelAuthorBase::withAuthor(
    Remodel& params,
    const Match& source,
    std::function<bool(DungeonAuthor&)> body,
    std::function<bool(const Match&)> acceptance)
{
    Match copy = source;
    MatchController controller(copy, params.codeset);
    bool result = false;
    LayoutFlyweight::getFlyweights().accessConst(copy.dungeon.layout, [&](const LayoutFlyweight& fw) {
        fw.layout.accessConst([&](const iLayout& layoutIntf) {
            layoutIntf.setupAdjacencyPointers(copy.dungeon.rooms);
            DungeonAuthor author(controller, layoutIntf);
            if (body(author))
                result = acceptance(copy);
        });
    });
    return result;
}

bool RemodelAuthorSetup4x1Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setup4x1Room(coords); }, accept);
}

bool RemodelAuthorSetup2x5Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setup2x5Room(coords); }, accept);
}

bool RemodelAuthorSetup3x3Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setup3x3Room(coords); }, accept);
}

bool RemodelAuthorSetupDoorway::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupDoorway(coords, dir); }, accept);
}

bool RemodelAuthorSetupExitDoor::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupExitDoor(coords, dir); }, accept);
}

bool RemodelAuthorSetupJailer::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupJailer(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupKeeper::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupKeeper(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupLadderUp::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupLadderUp(coords, dir); }, accept);
}

bool RemodelAuthorSetupLightningRodRoom::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupLightningRodRoom(coords, isCubed, isAwakened); }, accept);
}

bool RemodelAuthorSetupPoleUp::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupPoleUp(coords, dir); }, accept);
}

bool RemodelAuthorSetupPowerGeneratorRoom::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupPowerGeneratorRoom(coords); }, accept);
}

bool RemodelAuthorSetupCovenantDoor::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupCovenantDoor(coords, dir); }, accept);
}

bool RemodelAuthorSetupShifter::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupShifter(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupTimeGateRoomToFuture::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupTimeGateRoomToFuture(coords, isCubed, isAwakened); }, accept);
}

bool RemodelAuthorSetupTogglerBlue::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupTogglerBlue(coords, dir); }, accept);
}

bool RemodelAuthorSetupTogglerOrange::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupTogglerOrange(coords, dir); }, accept);
}

bool RemodelAuthorSetupTogglerSwitch::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupTogglerSwitch(coords, outCharacterId, outFloorId); }, accept);
}

bool RemodelAuthorSetupSacramentForgiveness::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupSacramentForgiveness(coords, outCharacterId, outFloorId); }, accept);
}

bool RemodelAuthorSetupElevatorColumn::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupElevatorColumn(elevatorRoomId, columnRoomIds, paidIndex); }, accept);
}

bool RemodelAuthorSetupHorizontalWalls::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupHorizontalWalls(row, y, z); }, accept);
}

bool RemodelAuthorSetupVerticalWalls::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonAuthor& a){ return a.setupVerticalWalls(row, y, z); }, accept);
}

bool RemodelAuthorApply::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, call, accept);
}

