#include "lifecycle_events.hpp"

namespace hider_seeker {

RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType type, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                                               uint8_t& outPublicEventCount) {
    // Not public under the ruleset: nothing is appended (not a rejection).
    if (!isDefinedPublicByRuleset) {
        return RuleOutcome::ACCEPTED;
    }

    // Append at the next free slot, i.e. index == current count. Slots below
    // outPublicEventCount hold events already emitted by earlier calls.
    if (!outPublicEvents.containsIndex(outPublicEventCount)) {
        return RuleOutcome::REJECTED_NO_SLOT_AVAILABLE;
    }

    outPublicEvents.assignValue(outPublicEventCount, PublicLifecycleEvent{type, tick});
    outPublicEventCount++;
    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker