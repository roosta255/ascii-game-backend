#include "Atom.hpp"
#include "AtomPlacement.hpp"
#include "Cardinal.hpp"
#include "iLayout.hpp"
#include "int4.hpp"
#include "LayoutGrid.hpp"
#include "SerpentineEmbedder.hpp"
#include "to_int4.hpp"

static Cardinal deltaToCardinal(int4 delta) {
    if (delta[0] > 0) return Cardinal::east();
    if (delta[0] < 0) return Cardinal::west();
    if (delta[1] > 0) return Cardinal::north();
    return Cardinal::south();
}

std::vector<AtomPlacement> SerpentineEmbedder::build(
    const std::vector<const Atom*>& atoms,
    const iLayout& layout,
    int /*seed*/) const
{
    const auto* grid = dynamic_cast<const LayoutGrid*>(&layout);
    if (!grid) return {};

    // Only x=0..(usableWidth-1) per row; skip trailing rooms if width % 4 != 0
    int usableWidth = (grid->width / 4) * 4;
    if (usableWidth == 0) return {};

    // Build serpentine walk within z=0 floor, each row alternating east/west
    std::vector<int> walk;
    for (int y = 0; y < grid->height; ++y) {
        bool goEast = (y % 2 == 0);
        if (goEast) {
            for (int x = 0; x < usableWidth; ++x)
                walk.push_back(grid->getIndex({x, y, 0, 0}));
        } else {
            for (int x = usableWidth - 1; x >= 0; --x)
                walk.push_back(grid->getIndex({x, y, 0, 0}));
        }
    }

    int needed = static_cast<int>(atoms.size());
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
