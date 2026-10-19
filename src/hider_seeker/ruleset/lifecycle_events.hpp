#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

enum class HiderSeekerEventType : uint8_t {
    SERVER_ADDED,
    SERVER_REMOVED,
    SERVER_PROBED
};

// PublicLifecycleEvent::outcome values (SERVER_PROBED only).
constexpr uint8_t PROBE_OUTCOME_NONE = 0;
constexpr uint8_t PROBE_OUTCOME_HIT = 1;
constexpr uint8_t PROBE_OUTCOME_MISS = 2;

// One tick's public events: one per applied intent, plus one SERVER_PROBED
// per wager (at most one wager per server).
constexpr uint8_t MAX_TICK_PUBLIC_EVENTS = MAX_INTENTS_PER_TICK + MAX_SERVERS;

struct PublicLifecycleEvent {
    HiderSeekerEventType type;
    uint32_t tick;
    uint8_t server_id = NO_SERVER_ID; // SERVER_PROBED only: the probed server
    uint8_t outcome = PROBE_OUTCOME_NONE; // SERVER_PROBED only: HIT or MISS
};

// Appends {type, tick} at outPublicEvents[outPublicEventCount] and increments
// outPublicEventCount, only when isDefinedPublicByRuleset is true. Callers may
// batch several events into the same array by reusing the same count. Returns
// REJECTED_NO_SLOT_AVAILABLE (appending nothing) if the array is already full.
RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS>& outPublicEvents,
                                               uint8_t& outPublicEventCount);

} // namespace hider_seeker