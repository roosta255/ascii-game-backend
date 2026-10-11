#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"

namespace hider_seeker {

struct IntentRecord {
    uint64_t receiptSequence;
    uint64_t idempotencyKey;
    uint8_t hiderId;
    HiderActionMode mode;
};

void orderAcceptedIntents(Array<IntentRecord, MAX_INTENTS_PER_TICK>& intents);
// consumedKeys holds consumedCount valid keys in slots [0, consumedCount);
// slots at or beyond consumedCount are unused and never compared, so every
// idempotencyKey value (including 0) is checked correctly.
bool isIntentAlreadyApplied(const Array<uint64_t, MAX_INTENTS_PER_TICK>& consumedKeys, int consumedCount, uint64_t idempotencyKey);

} // namespace hider_seeker