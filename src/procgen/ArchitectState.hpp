#pragma once

#include "Atom.hpp"

// Compile-time invariant tracker for atom sequencing.
// Tracks phase, key balance, and chest budget; never touches gameplay state.
struct ArchitectState {
    bool isBlueOpen  = true;
    int  keyBalance  = 0;
    int  chestsUsed  = 0;
    int  maxChests   = 0;  // 0 means unlimited

    bool canAccept(const AtomMeta& meta) const {
        if (meta.phaseSensitive && meta.requiresBlueOpen != isBlueOpen)
            return false;
        if (keyBalance + meta.keyProvided - meta.keyRequired < 0)
            return false;
        if (maxChests > 0 && chestsUsed + meta.chestsRequired > maxChests)
            return false;
        return true;
    }

    void apply(const AtomMeta& meta) {
        keyBalance += meta.keyProvided;
        keyBalance -= meta.keyConsumed;
        chestsUsed += meta.chestsRequired;
        if (meta.flipsPhase)
            isBlueOpen = !isBlueOpen;
    }
};
