#pragma once

// Audience projection layer (PROJ-001): builds the hider and public JSON views
// of a match from MatchState plus a ViewContext alone. Deliberately has no
// dependency on the api layer, the store, HTTP, or clocks.

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "public_event.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace hider_seeker {

constexpr uint32_t VIEW_VERSION = 3;

// Everything a view needs that is not part of MatchState.
struct ViewContext {
    std::string matchId;
    Array<std::string, MAX_HIDERS> accounts; // empty string == unbound slot
    std::vector<PublicEventEntry> publicEvents;
};

// The view of hider slot hiderId. Callers only pass a bound hiderId below
// state.hider_count; it is not validated here.
nlohmann::json buildHiderView(const MatchState& state, const ViewContext& ctx, uint8_t hiderId);
// The spectator view: no per-hider private data and nothing about the seeker.
nlohmann::json buildPublicView(const MatchState& state, const ViewContext& ctx);
// FNV-1a 64-bit of bytes as 16 lowercase hex digits.
std::string fnv1a64Hex(const std::string& bytes);

} // namespace hider_seeker
