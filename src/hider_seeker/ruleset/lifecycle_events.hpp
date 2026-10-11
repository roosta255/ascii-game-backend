#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"

namespace hider_seeker {

enum class HiderSeekerEventType : uint8_t {
    SERVER_ADDED,
    SERVER_REMOVED
};

struct PublicLifecycleEvent {
    HiderSeekerEventType type;
    uint32_t tick;
};

RuleOutcome emitPublicLifecycleEventIfDefined(HiderSeekerEventType, uint32_t tick, bool isDefinedPublicByRuleset,
                                               Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents);

} // namespace hider_seeker