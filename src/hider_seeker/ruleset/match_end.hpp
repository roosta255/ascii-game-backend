#pragma once

#include <cstdint>

namespace hider_seeker {

enum class MatchWinner : uint8_t { NONE = 0, HIDERS = 1, SEEKER = 2 };

} // namespace hider_seeker