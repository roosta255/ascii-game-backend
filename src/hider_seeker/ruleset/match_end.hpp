#pragma once

#include "../domain/state.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

enum class MatchWinner : uint8_t { NONE, HIDERS, SEEKER };

void checkMatchEnd(MatchState& match);

} // namespace hider_seeker