#pragma once

#include <unordered_map>

#include "Array.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "int4.hpp"
#include "Pointer.hpp"
#include "Room.hpp"

// rye <- ergot risk
// barley
// turnips
// parsnips
// nettles/sorrel
// lupins aka fava bean
// wild leek/wild garlic
// spelt?

class iLayout;

int4 build_room_map (
    Array<Room, DUNGEON_ROOM_COUNT>& rooms,
    const iLayout& layout,
    std::unordered_map<int4, Pointer<Room>>& shifted);
