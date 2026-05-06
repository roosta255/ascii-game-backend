#include "DungeonAuthor.hpp"
#include "Match.hpp"
#include "Remodel.hpp"
#include "RemodelAuthorSetBuilderStartingRoomId.hpp"

bool RemodelAuthorSetBuilderStartingRoomId::mutateMatch(
    Remodel& dst,
    const Match& source,
    const PathfindingCounter&,
    std::function<bool(const Match&)> acceptance) const
{
    return withAuthor(dst, source, [&](DungeonAuthor& author) {
        return author.setupBuilderStartingRoomId(builderIndex, coords);
    }, acceptance);
}
