#pragma once

#include "Cardinal.hpp"

// Stage 3: abstract placement of one 4-room atom.
// roomIds are dungeon room indices; no grid coordinates.
struct AtomPlacement {
    int      roomIds[4];
    Cardinal edgeDir;        // direction of the atom's internal edges (room[i] → room[i+1])
    int      prevExitRoomId; // -1 = first slot (no connection doorway needed)
    Cardinal connDir;        // direction from prevExitRoomId into roomIds[0]
};
