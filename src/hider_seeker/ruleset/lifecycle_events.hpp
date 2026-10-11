#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "rule_types.hpp"

namespace hider_seeker {

enum class HiderSeekerEventType : uint8_t {
    SERVER_ADDED,
    SERVER_REMOVED
};

struct PublicLifecycleEvent {
    HiderSeekerEventType type;
    uint32_t tick;
};

// Appends {type, tick} at outPublicEvents[outPublicEventCount] and increments
// outPublicEventCount, only when isDefinedPublicByRuleset is true. Callers may
// batch several events into the same array by reusing the same count. Returns
// REJECTED_NO_SLOT_AVAILABLE (appending nothing) if the array is already full.
RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                                               uint8_t& outPublicEventCount);

} // namespace hider_seeker