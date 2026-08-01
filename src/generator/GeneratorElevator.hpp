#pragma once

#include "Cardinal.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class GeneratorElevator {
public:
    constexpr static auto BOSS_ROOM_ID = 4;
    constexpr static auto ELEVATOR_ROOM_ID = 63;
    constexpr static auto ELEVATOR_EXIT_DIRECTION = Cardinal::south();
    constexpr static auto ELEVATOR_COLUMN_ROOM_IDS = Array<int, 7>(std::array<int, 7>{7, 16, 25, 34, 43, 52, 61});
    constexpr static auto ENTRANCE_ROOM_ID = 55;
    constexpr static auto FLOOR_7_CHEST_ROOM_ID = 62;
};
