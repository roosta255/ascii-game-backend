#pragma once

#include "iAtomEmbedder.hpp"

// Stage 1 compatibility: generates the same serpentine placements as the
// former hardcoded SLOTS[], computed from the layout's grid dimensions.
// Walks rows of 4 rooms east/west alternately within a single z=0 floor.
// Rows whose width is not a multiple of 4 have trailing rooms skipped.
class SerpentineEmbedder : public iAtomEmbedder {
public:
    std::vector<AtomPlacement> build(
        const std::vector<const Atom*>& atoms,
        const iLayout& layout,
        int seed) const override;
};
