#pragma once

#include "../../hider_seeker/domain/state.hpp"
#include "Array.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/rule_types.hpp"
#include "../ruleset/action_rules.hpp"

namespace hider_seeker {

enum class TickActionKind : uint8_t { HIDER_ACTION, ADD_SERVER, REMOVE_SERVER };

// IdempotencyKeyT: declare using the exact type from IntentRecord
using IdempotencyKeyT = decltype(IntentRecord{}.idempotencyKey);

struct TickIntent {
    TickActionKind kind;
    HiderActionMode action_mode;   // meaningful iff kind == HIDER_ACTION
    uint8_t target_server_id;      // meaningful iff kind == ADD_SERVER or kind == REMOVE_SERVER
    
    // Embed the existing IntentRecord unchanged
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
            if (keys[i] == key) {
                return true;
            }
        }
        return false;
    }
};

constexpr uint8_t MAX_PROBES_PER_TICK = MAX_SERVERS; // reusing the frozen MAX_SERVERS=16 constant

struct TickOutput {
    Array<IdempotencyKeyT, MAX_INTENTS_PER_TICK> consumedKeys; 
    uint8_t consumedKeyCount = 0;
    
    Array<IntentOutcomeRecord, MAX_INTENTS_PER_TICK> intentOutcomes; 
    uint8_t intentOutcomeCount = 0;
    
    Array<ProbeResult, MAX_PROBES_PER_TICK> probeResults;
    Array<ProbeConsequences, MAX_PROBES_PER_TICK> probeConsequences;
    uint8_t probeCount = 0;
};

} // namespace hider_seeker