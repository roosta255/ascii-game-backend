#include "AtomLibrary.hpp"

using E = AtomEdgeType;

// AtomMeta fields: phaseSensitive, requiresBlueOpen, flipsPhase,
//                  keyRequired, keyProvided, keyConsumed, chestsRequired

static const Atom ATOMS[] = {
    {
        "DOORWAY",
        { E::DOORWAY, E::DOORWAY, E::DOORWAY },
        { false, false, false, false },
        { false, false, false, false },
        { false, true, false, 0, 0, 0 }
    },
    // KEY_PICKUP: chest at node[1] grants one key, no gate.
    {
        "KEY_PICKUP",
        { E::DOORWAY, E::DOORWAY, E::DOORWAY },
        { false, true, false, false },
        { false, false, false, false },
        { false, true, false, 0, 1, 0, 1 }
    },
    // KEEPER_SIMPLE: player must arrive with 1 key; gate at edge[2].
    {
        "KEEPER_SIMPLE",
        { E::DOORWAY, E::DOORWAY, E::KEEPER_KEYED },
        { false, false, false, false },
        { false, false, false, false },
        { false, true, false, 1, 0, 1 }
    },
    // PHASE_FLIP: toggler switch at node[1] inverts global phase.
    {
        "PHASE_FLIP",
        { E::DOORWAY, E::DOORWAY, E::DOORWAY },
        { false, false, false, false },
        { false, true, false, false },
        { false, true, true, 0, 0, 0 }
    },
    // TOGGLER_BLUE: door at edge[0] is BLUE_CLOSED; needs blue phase active.
    {
        "TOGGLER_BLUE",
        { E::TOGGLER_BLUE, E::DOORWAY, E::DOORWAY },
        { false, false, false, false },
        { false, false, false, false },
        { true, true, false, 0, 0, 0 }
    },
    // TOGGLER_ORANGE: door at edge[0] is ORANGE_OPEN; needs orange phase (isBlueOpen=false).
    {
        "TOGGLER_ORANGE",
        { E::TOGGLER_ORANGE, E::DOORWAY, E::DOORWAY },
        { false, false, false, false },
        { false, false, false, false },
        { true, false, false, 0, 0, 0 }
    },
};

static constexpr int ATOM_COUNT = static_cast<int>(sizeof(ATOMS) / sizeof(ATOMS[0]));

const Atom* AtomLibrary::getAll(int& outCount) {
    outCount = ATOM_COUNT;
    return ATOMS;
}

std::vector<const Atom*> AtomLibrary::getCompatible(const ArchitectState& state) {
    std::vector<const Atom*> result;
    for (int i = 0; i < ATOM_COUNT; ++i) {
        if (state.canAccept(ATOMS[i].meta))
            result.push_back(&ATOMS[i]);
    }
    return result;
}
