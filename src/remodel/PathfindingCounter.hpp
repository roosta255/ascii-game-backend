#pragma once

#include "ActionEnum.hpp"
#include "Array.hpp"
#include "Cardinal.hpp"
#include "Bitstick.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include <functional>

struct PathfindingCounter {
    struct ActionCounter {
        int count;
        Bitstick<DUNGEON_ROOM_COUNT> rooms;
        Array<Bitstick<DUNGEON_ROOM_COUNT>, 4> doors;
    };
    Array<ActionCounter, ACTION_COUNT> actionCounter;
    Array<ActionCounter, ACTION_TYPE_COUNT> actionTypeCounter;
    ActionCounter doorwayMoveCounter, nonDoorwayMoveCounter;

    bool loopDoorways(std::function<bool(const int&, const Cardinal&)> consumer, const int seed) const;
    bool loopRooms(std::function<bool(const int&)> consumer, const int seed) const;
};
