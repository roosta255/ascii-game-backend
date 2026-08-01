#pragma once
#include <array>
#include <cstdint>
#include "Bitstick.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"

struct FieldState {
    std::array<int16_t, DUNGEON_ROOM_COUNT> values{};
    Bitstick<(size_t)DUNGEON_ROOM_COUNT> flags;
};
