#include "DungeonAuthor.hpp"
#include "Match.hpp"
#include "Remodel.hpp"
#include "RemodelAuthorChestSeeder.hpp"

bool RemodelAuthorChestSeeder::mutateMatch(
    Remodel& dst,
    const Match& source,
    const PathfindingCounter&,
    std::function<bool(const Match&)> acceptance) const
{
    return withAuthor(dst, source, [&](DungeonAuthor& author) {
        return author.setupChest(chest.room, {chest.lock, chest.critterRole, chest.items});
    }, acceptance);
}
