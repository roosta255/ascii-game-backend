#pragma once

#include "iAtomEmbedder.hpp"

// Stage 4: BFS/directed-walk embedder.
// Builds a linear path through the layout by following a preferred direction
// and turning when blocked, using a visited-set to prevent revisits.
// Naturally adapts to any rectangular grid without hardcoded coordinates.
// Cross-floor (depth-axis) support is reserved for a future extension.
class GraphPathEmbedder : public iAtomEmbedder {
public:
    std::vector<AtomPlacement> build(
        const std::vector<const Atom*>& atoms,
        const iLayout& layout,
        int seed) const override;
};
