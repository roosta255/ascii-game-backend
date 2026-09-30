#pragma once

#include "state.hpp"
#include <vector>

namespace hider_seeker {

// The schema_version saveMatchState/loadMatchState currently write/require. Exposed so
// callers that construct a fresh MatchState (e.g. the API layer's match-creation path)
// have a single source of truth instead of duplicating the version number.
uint32_t currentSchemaVersion();

void saveMatchState(const MatchState& state, std::vector<uint8_t>& out);
/// @brief Loads match state from a byte buffer.
/// @param in The input buffer to read from
/// @param state The output state object to populate
/// @return true if the buffer was successfully parsed, false otherwise.
///         On false return, the contents of `state` are unspecified and must not be read.
bool loadMatchState(const std::vector<uint8_t>& in, MatchState& state);

} // namespace hider_seeker