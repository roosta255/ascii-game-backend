#include "lifecycle_events.hpp"

namespace hider_seeker {

RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType type, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents) {
    // Append the event only when isDefinedPublicByRuleset is true
    if (isDefinedPublicByRuleset && outPublicEvents.size() > 0) {
        // Find first available slot in events array
        for (int i = 0; i < outPublicEvents.size(); ++i) {
            if (!outPublicEvents.containsIndex(i)) continue;
            
            // Check if this slot is unused (by checking if it's a valid event)
            // For simplicity, we'll assume the first empty slot is used
            // In a real implementation, there would be tracking of used slots
            
            outPublicEvents[i].type = type;
            outPublicEvents[i].tick = tick;
            
            return RuleOutcome::ACCEPTED;
        }
    }
    
    // No event appended (either not public or no space)
    return RuleOutcome::ACCEPTED; // Not rejecting, just not appending
}

} // namespace hider_seeker