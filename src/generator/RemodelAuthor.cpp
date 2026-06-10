#include "DungeonMutator.hpp"
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
    std::function<bool(DungeonMutator&)> body,
    std::function<bool(const Match&)> acceptance)
{
    Match copy = source;
    MatchController controller(copy, params.codeset);
    bool result = false;
    LayoutFlyweight::getFlyweights().accessConst(copy.dungeon.layout, [&](const LayoutFlyweight& fw) {
        fw.layout.accessConst([&](const iLayout& layoutIntf) {
            layoutIntf.setupAdjacencyPointers(copy.dungeon.rooms);
            DungeonMutator mutator(controller, layoutIntf);
            if (body(mutator))
                result = acceptance(copy);
        });
    });
    return result;
}

bool RemodelAuthorSetup4x1Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setup4x1Room(coords); }, accept);
}

bool RemodelAuthorSetup2x5Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setup2x5Room(coords); }, accept);
}

bool RemodelAuthorSetup3x3Room::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setup3x3Room(coords); }, accept);
}

bool RemodelAuthorSetupDoorway::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupDoorway(coords, dir); }, accept);
}

bool RemodelAuthorSetupExitDoor::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupExitDoor(coords, dir); }, accept);
}

bool RemodelAuthorSetupJailer::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupJailer(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupKeeper::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupKeeper(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupLadderUp::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupLadderUp(coords, dir); }, accept);
}

bool RemodelAuthorSetupLightningRodRoom::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupLightningRodRoom(coords, isCubed, isAwakened); }, accept);
}

bool RemodelAuthorSetupPoleUp::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupPoleUp(coords, dir); }, accept);
}

bool RemodelAuthorSetupPowerGeneratorRoom::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupPowerGeneratorRoom(coords); }, accept);
}

bool RemodelAuthorSetupCovenantDoor::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupCovenantDoor(coords, dir); }, accept);
}

bool RemodelAuthorSetupShifter::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupShifter(coords, dir, isKeyed); }, accept);
}

bool RemodelAuthorSetupTimeGateRoomToFuture::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupTimeGateRoomToFuture(coords, isCubed, isAwakened); }, accept);
}

bool RemodelAuthorSetupTogglerBlue::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupTogglerBlue(coords, dir); }, accept);
}

bool RemodelAuthorSetupTogglerOrange::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupTogglerOrange(coords, dir); }, accept);
}

bool RemodelAuthorSetupTogglerSwitch::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupTogglerSwitch(coords, outCharacterId, outFloorId); }, accept);
}

bool RemodelAuthorSetupSacramentForgiveness::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupSacramentForgiveness(coords, outCharacterId, outFloorId); }, accept);
}

bool RemodelAuthorAllocateCharacter::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.allocateCharacter(coords, role, outCharacterId, outFloorId); }, accept);
}

bool RemodelAuthorSetupElevatorColumn::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupElevatorColumn(elevatorRoomId, columnRoomIds, paidIndex); }, accept);
}

bool RemodelAuthorSetupHorizontalWalls::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupHorizontalWalls(row, y, z); }, accept);
}

bool RemodelAuthorSetupVerticalWalls::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, [&](DungeonMutator& m){ return m.setupVerticalWalls(row, y, z); }, accept);
}

bool RemodelAuthorApply::mutateMatch(Remodel& p, const Match& src, const PathfindingCounter&, std::function<bool(const Match&)> accept) const {
    return withAuthor(p, src, call, accept);
}
