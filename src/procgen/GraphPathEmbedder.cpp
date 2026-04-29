#include "Atom.hpp"
#include "AtomPlacement.hpp"
#include "Cardinal.hpp"
#include "GraphPathEmbedder.hpp"
#include "iLayout.hpp"
#include "int4.hpp"
#include "LayoutGrid.hpp"
#include "to_int4.hpp"
#include <vector>

static Cardinal deltaToCardinal(int4 delta) {
    if (delta[0] > 0) return Cardinal::east();
    if (delta[0] < 0) return Cardinal::west();
    if (delta[1] > 0) return Cardinal::north();
    return Cardinal::south();
}

// Walk the grid in a directed serpentine: go primary until blocked, step
// secondary and flip primary, repeat. Returns room indices in walk order.
static std::vector<int> buildWalk(const LayoutGrid& grid, int count) {
    int totalRooms = grid.width * grid.height * grid.depth * grid.times;
    std::vector<bool> visited(totalRooms, false);
    std::vector<int>  path;
    path.reserve(count);

    int4     pos     = {0, 0, 0, 0};
    Cardinal primary = Cardinal::east();

    while (static_cast<int>(path.size()) < count) {
        int idx = grid.getIndex(pos);
        visited[idx] = true;
        path.push_back(idx);

        // Try primary direction
        auto offset = to_int4(primary.getRectOffset());
        int4 nextPrimary = pos + offset;
        if (grid.contains(nextPrimary) && !visited[grid.getIndex(nextPrimary)]) {
            pos = nextPrimary;
            continue;
        }

        // Try north (secondary) and flip primary
        int4 nextSecondary = pos + int4{0, 1, 0, 0};
        if (grid.contains(nextSecondary) && !visited[grid.getIndex(nextSecondary)]) {
            pos     = nextSecondary;
            primary = primary.getFlip();
            continue;
        }

        break; // no valid move
    }
    return path;
}

std::vector<AtomPlacement> GraphPathEmbedder::build(
    const std::vector<const Atom*>& atoms,
    const iLayout& layout,
    int /*seed*/) const
{
    const auto* grid = dynamic_cast<const LayoutGrid*>(&layout);
    if (!grid) return {};

    int needed = static_cast<int>(atoms.size());
    std::vector<int> walk = buildWalk(*grid, needed * 4);

    std::vector<AtomPlacement> result;
    int walkIdx = 0;

    for (int slot = 0; slot < needed && walkIdx + 4 <= static_cast<int>(walk.size()); ++slot) {
        AtomPlacement p;
        for (int n = 0; n < 4; ++n)
            p.roomIds[n] = walk[walkIdx++];

        int4 c0 = grid->getCoordinates(p.roomIds[0]);
        int4 c1 = grid->getCoordinates(p.roomIds[1]);
        p.edgeDir = deltaToCardinal(c1 - c0);

        if (result.empty()) {
            p.prevExitRoomId = -1;
            p.connDir        = p.edgeDir;
        } else {
            p.prevExitRoomId = result.back().roomIds[3];
            int4 prev        = grid->getCoordinates(p.prevExitRoomId);
            int4 curr        = grid->getCoordinates(p.roomIds[0]);
            p.connDir        = deltaToCardinal(curr - prev);
        }
        result.push_back(p);
    }
    return result;
}
