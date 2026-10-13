#include "regeneration.hpp"
#include "../hider_seeker/domain/state.hpp"

namespace hider_seeker {

void regenerateForTick(MatchState& state) {
    // Regenerate action points for hiders
    for (auto& hider : state.hiders) {
        if (hider.active && hider.status != HIDER_STATUS_CAPTURED) {
            // Add AP regen, capping at PLACEHOLDER_AP_CAP
            hider.action_points += PLACEHOLDER_AP_REGEN_PER_TICK;
            if (hider.action_points > PLACEHOLDER_AP_CAP) {
                hider.action_points = PLACEHOLDER_AP_CAP;
            }
        }
    }
    
    // Regenerate seeker connection tokens
    if (state.seeker.connection_tokens < state.seeker.max_tokens) {
        state.seeker.connection_tokens += PLACEHOLDER_SEEKER_TOKEN_REGEN_PER_TICK;
        if (state.seeker.connection_tokens > state.seeker.max_tokens) {
            state.seeker.connection_tokens = state.seeker.max_tokens;
        }
    }
    
    // LOA and other fields are left unchanged
}

} // namespace hider_seeker