#pragma once

// Wire spellings of game enums, shared by the api layer and the projection
// layer so the move API and the audience views never disagree on a name.

#include "../domain/state.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/lifecycle_events.hpp"
#include "../ruleset/match_end.hpp"
#include "../ruleset/goal_rules.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/server_metrics.hpp"
#include <cstdint>
#include <string>

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

inline std::string_view winnerName(MatchWinner winner) {
    switch (winner) {
        case MatchWinner::NONE:   return "";
        case MatchWinner::SEEKER: return "SEEKER";
        case MatchWinner::HIDERS: return "HIDERS";
    }
    return "";
}

// Returns the string name for a GOAL_STATUS value.
inline std::string goalStatusName(uint8_t status) {
    switch (status) {
        case GOAL_STATUS_OPEN:                 return "OPEN";
        case GOAL_STATUS_COMPLETED:            return "COMPLETED";
        case GOAL_STATUS_CANCELLED_SERVER_REMOVED: return "CANCELLED";
    }
    return "UNKNOWN";
}

// Returns the string name for a RequirementType value, without the REQ_ prefix.
inline std::string requirementTypeName(uint8_t type) {
    switch (type) {
        case REQ_RESOURCE_MIN:              return "RESOURCE_MIN";
        case REQ_RESOURCE_SPEND:            return "RESOURCE_SPEND";
        case REQ_ACTIVE_SERVER_COUNT:       return "ACTIVE_SERVER_COUNT";
        case REQ_SERVER_METRIC_THRESHOLD:   return "SERVER_METRIC_THRESHOLD";
        case REQ_ACTION_COUNT:              return "ACTION_COUNT";
        case REQ_CONFIGURED_SERVER:         return "CONFIGURED_SERVER";
        case REQ_CUSTOM_RULE:               return "CUSTOM_RULE";
    }
    return "UNKNOWN";
}

} // namespace hider_seeker