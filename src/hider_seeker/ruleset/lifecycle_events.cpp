#include "lifecycle_events.hpp"

namespace hider_seeker {

RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType type, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents) {
    // Append the event only when isDefinedPublicByRuleset is true
    if (isDefinedPublicByRuleset) {
        // Find first available slot in events array - just use index 0 for now
        // In a real implementation, there would be proper tracking of used slots
        if (outPublicEvents.size() > 0) {
            outPublicEvents.assignValue(0, PublicLifecycleEvent{type, tick});
            return RuleOutcome::ACCEPTED;
        }
    }
    
    // No event appended (either not public or no space)
    return RuleOutcome::ACCEPTED; // Not rejecting, just not appending
}

} // namespace hider_seeker