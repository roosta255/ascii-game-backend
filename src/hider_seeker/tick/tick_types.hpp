#pragma once

#include "../../hider_seeker/domain/state.hpp"
#include "Array.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/rule_types.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/lifecycle_events.hpp"

namespace hider_seeker {

enum class TickActionKind : uint8_t { HIDER_ACTION, ADD_SERVER, REMOVE_SERVER };

// IdempotencyKeyT: declare using the exact type from IntentRecord
using IdempotencyKeyT = decltype(IntentRecord{}.idempotencyKey);

struct TickIntent {
    TickActionKind kind;
    uint8_t target_server_id;  // meaningful iff kind == ADD_SERVER or kind == REMOVE_SERVER
    
    // Embed the existing IntentRecord unchanged; for HIDER_ACTION,
    // intent.mode is the action to apply.
    IntentRecord intent;
};

struct IntentOutcomeRecord { 
    IdempotencyKeyT idempotencyKey; 
    RuleOutcome outcome; 
};

constexpr uint16_t MAX_TRACKED_CONSUMED_KEYS = 4096; // new tick-layer constant

struct ConsumedKeySet { 
    Array<IdempotencyKeyT, MAX_TRACKED_CONSUMED_KEYS> keys; 
    uint16_t count = 0; 
    
    bool contains(IdempotencyKeyT key) const {
        for (int i = 0; i < count; ++i) {
            if (keys.getOrDefault(i, 0) == key) {
                return true;
            }
        }
        return false;
    }
};

constexpr uint8_t MAX_PROBES_PER_TICK = MAX_SERVERS; // reusing the frozen MAX_SERVERS=16 constant

// Each applied intent emits at most one public event, so one
// tick's events always fit.
static_assert(MAX_EVENTS >= MAX_INTENTS_PER_TICK,
              "TickOutput::publicEvents must hold one event per intent");

struct TickOutput {
    Array<IdempotencyKeyT, MAX_INTENTS_PER_TICK> consumedKeys; 
    uint8_t consumedKeyCount = 0;
    
    Array<IntentOutcomeRecord, MAX_INTENTS_PER_TICK> intentOutcomes; 
    uint8_t intentOutcomeCount = 0;
    
    Array<ProbeResult, MAX_PROBES_PER_TICK> probeResults;
    Array<ProbeConsequences, MAX_PROBES_PER_TICK> probeConsequences;
    uint8_t probeCount = 0;
    
    // Public lifecycle events from ADD_SERVER/REMOVE_SERVER, in the
    // order the intents were applied.
    Array<PublicLifecycleEvent, MAX_EVENTS> publicEvents;
    uint8_t publicEventCount = 0;
};

} // namespace hider_seeker