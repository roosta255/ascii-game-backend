#include "regeneration.hpp"
#include "probe_rules.hpp"

namespace hider_seeker {

namespace {

// Adds amount to value without exceeding cap. A value already above cap is
// left as it is (regeneration never lowers a resource).
uint8_t saturatingAdd(uint8_t value, uint8_t amount, uint8_t cap) {
    if (value >= cap) {
        return value;
    }
    const uint16_t sum = static_cast<uint16_t>(value) + amount;
    return sum > cap ? cap : static_cast<uint8_t>(sum);
}

} // namespace

void regenerateForTick(MatchState& state) {
    // PLACEHOLDER_AP_CAP (10) is at least as large as the largest known AP cost
    // (PAY_GOAL at 10), so every action remains reachable through regeneration.
    for (auto& hider : state.hiders) {
        if (hider.active && hider.status != HIDER_STATUS_CAPTURED) {
            hider.action_points = saturatingAdd(hider.action_points, PLACEHOLDER_AP_REGEN_PER_TICK,
                                                PLACEHOLDER_AP_CAP);
        }
    }

    state.seeker.connection_tokens = saturatingAdd(state.seeker.connection_tokens,
                                                   PLACEHOLDER_SEEKER_TOKEN_REGEN_PER_TICK,
                                                   state.seeker.max_tokens);

    // LOA (hider resources) and every other field are left unchanged.
}

} // namespace hider_seeker
