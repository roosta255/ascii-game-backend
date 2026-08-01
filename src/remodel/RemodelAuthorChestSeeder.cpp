#include "DungeonMutator.hpp"
#include "Match.hpp"
#include "Remodel.hpp"
#include "RemodelAuthorChestSeeder.hpp"

bool RemodelAuthorChestSeeder::mutateMatch(
    Remodel& dst,
    const Match& source,
    const PathfindingCounter&,
    std::function<bool(const Match&)> acceptance) const
{
    return withAuthor(dst, source, [&](DungeonMutator& m) {
        return m.setupChest(chest.room, {chest.lock, chest.critterRole, chest.containedRole, chest.items});
    }, acceptance);
}
