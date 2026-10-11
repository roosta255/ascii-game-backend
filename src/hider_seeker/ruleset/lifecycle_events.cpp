#include "lifecycle_events.hpp"

namespace hider_seeker {

RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType type, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents) {
    // Append the event only when isDefinedPublicByRuleset is true
    if (isDefinedPublicByRuleset) {
        // Find first available slot in events array - properly track used slots
        for (int i = 0; i < outPublicEvents.size(); ++i) {
            bool isEmpty = outPublicEvents.accessConst(i, [](const PublicLifecycleEvent& event) {
                return event.type == static_cast<HiderSeekerEventType>(0xFF); // Assuming this is empty marker
            }).value_or(true);
            
            if (isEmpty) {
                outPublicEvents.assignValue(i, PublicLifecycleEvent{type, tick});
                return RuleOutcome::ACCEPTED;
            }
        }
    }
    
    // No event appended (either not public or no space)
    return RuleOutcome::ACCEPTED; // Not rejecting, just not appending
}

} // namespace hider_seeker