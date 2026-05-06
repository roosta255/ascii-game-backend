#include "DungeonGenerator.hpp"
#include "Match.hpp"
#include "Remodel.hpp"
#include "RemodelAuthorRunGeneratorLoop.hpp"

bool RemodelAuthorRunGeneratorLoop::mutateMatch(
    Remodel& dst,
    const Match& source,
    const PathfindingCounter&,
    std::function<bool(const Match&)> acceptance) const
{
    DungeonGenerator generator(dst.playerId, dst.characterId, dst.bossRoomId, dst.codeset);
    Maybe<Match> result = generator.remodelLoop(source, iterations);

    bool accepted = false;
    result.accessConst([&](const Match& created) {
        accepted = acceptance(created);
    });
    return accepted;
}
