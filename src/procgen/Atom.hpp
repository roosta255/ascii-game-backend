#pragma once

enum class AtomEdgeType {
    DOORWAY,
    KEEPER_KEYED,
    TOGGLER_BLUE,
    TOGGLER_ORANGE,
};

struct AtomMeta {
    bool phaseSensitive   = false;
    bool requiresBlueOpen = true;
    bool flipsPhase       = false;
    int  keyRequired      = 0;
    int  keyProvided      = 0;
    int  keyConsumed      = 0;
    int  chestsRequired   = 0;
};

// Linear 4-node atom. Nodes are indexed 0..3, connected in sequence.
// edges[i] is the door type between node[i] and node[i+1].
// nodeHasKeyChest[i]  – allocate a key chest in that node's room.
// nodeHasToggler[i]   – place a toggler switch in that node's room.
struct Atom {
    const char*  name;
    AtomEdgeType edges[3];
    bool         nodeHasKeyChest[4];
    bool         nodeHasToggler[4];
    AtomMeta     meta;
};
