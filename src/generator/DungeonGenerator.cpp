#include "ActionType.hpp"
#include "Codeset.hpp"
#include "DungeonGenerator.hpp"
#include "iRemodel.hpp"
#include "Remodel.hpp"
#include "RemodelFlyweight.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"

DungeonGenerator::DungeonGenerator(const std::string& playerId, const int& runCharacterId, const int& bossRoomId, Codeset& codeset)
    : playerId(playerId), runCharacterId(runCharacterId), bossRoomId(bossRoomId), codeset(codeset) {}

Bitstick<REMODEL_COUNT> getBlockers() {
    Bitstick<REMODEL_COUNT> result;
    for (int i = 0; i < REMODEL_COUNT; i++) {
        RemodelFlyweight::getFlyweights().accessConst(i, [&](const RemodelFlyweight& flyweight){
            if (flyweight.isBlocker) {
                result.setIndexOn(i);
            }
        });
    }
    return result;
}

// functions
Maybe<Match> DungeonGenerator::remodel(const Match& original) {
    const auto originalCounter = Pathfinder::build(playerId, runCharacterId, bossRoomId, original, codeset).getCounter();

    const auto actionMinimum = originalCounter.actionTypeCounter.getPointer(ACTION_TYPE_USAGE).map<int>(
        [&](const PathfindingCounter::ActionCounter& counter){
            return counter.count;
        }).orElse(0);
    const auto minimumNonDoorwayMoveCount = originalCounter.nonDoorwayMoveCounter.count;
    codeset.setTable(CODE_DUNGEON_GENERATOR_REMODEL_ACTION_MINIMUM, actionMinimum);

    Remodel params {
        .playerId = playerId,
        .characterId = runCharacterId,
        .bossRoomId = bossRoomId,
        .codeset = codeset,
        .allowlist = allowlist,
        .blocklist = blocklist,
        .acceptance = [&](const Match& proposal, const PathfindingCounter& counter){
            const auto proposedCounter = Pathfinder::build(playerId, runCharacterId, bossRoomId, proposal, codeset).getCounter();

            const auto proposedCount = proposedCounter.actionTypeCounter.getPointer(ACTION_TYPE_USAGE).map<int>(
                [&](const PathfindingCounter::ActionCounter& counter){
                    return counter.count;
                }).orElse(0);
            const auto proposedNonDoorwayMoveCount = originalCounter.nonDoorwayMoveCounter.count;
            codeset.setTable(CODE_DUNGEON_GENERATOR_REMODEL_PROPOSED_ACTIONS, proposedCount);
            return proposedCount > actionMinimum || proposedCount == actionMinimum && proposedNonDoorwayMoveCount > minimumNonDoorwayMoveCount;
        }
    };

    static const auto allBlockers = getBlockers();
    const auto blockers = allowlist.map<Bitstick<REMODEL_COUNT>>(
        [&](const Bitstick<REMODEL_COUNT>& allowlist){
            return allowlist & allBlockers;
        }).orElse(allBlockers) - blocklist.orElse(Bitstick<REMODEL_COUNT>());

    if (codeset.addFailure(blockers.isEmpty(), CODE_DUNGEON_GENERATOR_HAS_NO_APPLICABLE_REMODELS)) {
        return Maybe<Match>::empty();
    }

    Maybe<Match> result;
    for (int i = 0; i < REMODEL_COUNT && result.isEmpty(); i++) {
        if (blockers[i]) {
            RemodelFlyweight::getFlyweights().accessConst(i, [&](const RemodelFlyweight& flyweight){
                flyweight.remodel.accessConst([&](const iRemodel& fixerRemodel){
                    result = fixerRemodel.buildMatch(params, original);
                });
            });
        }
    }

    if (codeset.addFailure(result.isEmpty(), CODE_DUNGEON_GENERATOR_REMODEL_WITHOUT_RESULT)) {
        return Maybe<Match>::empty();
    }

    // already validated result presence
    const auto resultIsSolved = result.map<bool>([&](const Match& resultMatch){
        const auto resultPath = Pathfinder::build(playerId, runCharacterId, bossRoomId, resultMatch, codeset);
        return !codeset.addFailure(!resultPath.isSolved, CODE_DUNGEON_GENERATOR_REMODEL_NOT_SOLVABLE);
    }).orElse(false);

    return result;
}

Maybe<Match> DungeonGenerator::remodelLoop(const Match& original, const int loops) {
    Match result = original;
    for (int i = 0; i < loops; i++) {
        Maybe<Match> temp = remodel(result);
        if (codeset.addFailure(temp.isEmpty(), CODE_DUNGEON_GENERATOR_FAILED_REMODEL_LOOP)) {
            return Maybe<Match>::empty();
        }
        temp.accessConst([&](const Match& next){
            result = next;
        });
    }
    return result;
}
