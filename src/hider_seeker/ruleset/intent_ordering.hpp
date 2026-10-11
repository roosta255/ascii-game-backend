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
bool isIntentAlreadyApplied(const Array<uint64_t, MAX_INTENTS_PER_TICK>& consumedKeys, uint64_t idempotencyKey);

} // namespace hider_seeker