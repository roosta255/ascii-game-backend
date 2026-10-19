#pragma once

// Wire spellings of game enums, shared by the api layer and the projection
// layer so the move API and the audience views never disagree on a name.

#include "../domain/state.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/lifecycle_events.hpp"
#include "../ruleset/match_end.hpp"
#include <cstdint>

namespace hider_seeker {

inline const char* phaseName(uint8_t phase) {
    switch (phase) {
        case PHASE_LOBBY:    return "LOBBY";
        case PHASE_ACTIVE:   return "ACTIVE";
        case PHASE_COMPLETE: return "COMPLETE";
    }
    return "UNKNOWN";
}

inline const char* actionModeName(HiderActionMode mode) {
    const char* name = hiderActionName(mode);
    return name ? name : "UNKNOWN";
}

inline const char* eventTypeName(HiderSeekerEventType type) {
    switch (type) {
        case HiderSeekerEventType::SERVER_ADDED:   return "SERVER_ADDED";
        case HiderSeekerEventType::SERVER_REMOVED: return "SERVER_REMOVED";
        case HiderSeekerEventType::SERVER_PROBED:  return "SERVER_PROBED";
    }
    return "UNKNOWN";
}

inline const char* probeOutcomeName(uint8_t outcome) {
    switch (outcome) {
        case PROBE_OUTCOME_HIT:  return "HIT";
        case PROBE_OUTCOME_MISS: return "MISS";
    }
    return "";
}

inline const char* winnerName(MatchWinner winner) {
    switch (winner) {
        case MatchWinner::NONE:   return nullptr;
        case MatchWinner::SEEKER: return "SEEKER";
        case MatchWinner::HIDERS: return "HIDERS";
    }
    return nullptr;
}

} // namespace hider_seeker
