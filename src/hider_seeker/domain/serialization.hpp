#pragma once

#include "state.hpp"
#include <vector>

namespace hider_seeker {

void saveMatchState(const MatchState& state, std::vector<uint8_t>& out);
bool loadMatchState(const std::vector<uint8_t>& in, MatchState& state);

} // namespace hider_seeker