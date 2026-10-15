#pragma once

// Wire spellings of game enums, shared by the api layer and the projection
// layer so the move API and the audience views never disagree on a name.

#include "../domain/state.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/lifecycle_events.hpp"
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
    switch (mode) {
        case HiderActionMode::ADD_SERVER:       return "ADD_SERVER";
        case HiderActionMode::REMOVE_SERVER:    return "REMOVE_SERVER";
        case HiderActionMode::CONFIGURE_SERVER: return "CONFIGURE_SERVER";
        case HiderActionMode::SUPPRESS_ERROR:   return "SUPPRESS_ERROR";
        case HiderActionMode::BOOST_CPU:        return "BOOST_CPU";
        case HiderActionMode::ROTATE_ROUTE:     return "ROTATE_ROUTE";
        case HiderActionMode::PAY_GOAL:         return "PAY_GOAL";
    }
    return "UNKNOWN";
}

inline const char* eventTypeName(HiderSeekerEventType type) {
    switch (type) {
        case HiderSeekerEventType::SERVER_ADDED:   return "SERVER_ADDED";
        case HiderSeekerEventType::SERVER_REMOVED: return "SERVER_REMOVED";
    }
    return "UNKNOWN";
}

} // namespace hider_seeker
