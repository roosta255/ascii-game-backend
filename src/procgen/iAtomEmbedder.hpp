#pragma once

#include <vector>

struct Atom;
struct AtomPlacement;
class iLayout;

// Stage 2: interface that maps an atom sequence onto concrete room placements.
// Atoms are chosen first; the embedder assigns room IDs after.
class iAtomEmbedder {
public:
    virtual ~iAtomEmbedder() = default;

    virtual std::vector<AtomPlacement> build(
        const std::vector<const Atom*>& atoms,
        const iLayout& layout,
        int seed) const = 0;
};
