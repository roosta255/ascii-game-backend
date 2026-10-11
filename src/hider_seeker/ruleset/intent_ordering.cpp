#include "intent_ordering.hpp"
#include <algorithm>

namespace hider_seeker {

void orderAcceptedIntents(Array<IntentRecord, MAX_INTENTS_PER_TICK>& intents) {
    // Sort strictly ascending by receiptSequence only
    // Since receipt sequence is a durably-assigned monotonic counter,
    // no tie-break rule is needed because two intents cannot share a sequence value
    
    std::sort(intents.begin(), intents.end(), 
        [](const IntentRecord& a, const IntentRecord& b) {
            return a.receiptSequence < b.receiptSequence;
        });
}

bool isIntentAlreadyApplied(const Array<uint64_t, MAX_INTENTS_PER_TICK>& consumedKeys, int consumedCount, uint64_t idempotencyKey) {
    // Pure function that checks if an intent has already been applied
    // This module does not itself persist consumed keys since MatchState (frozen)
    // has no field for them

    // Only the first consumedCount slots hold valid consumed keys; unused slots
    // are never compared, so a legitimate key of 0 is handled like any other.
    const int limit = std::min(std::max(consumedCount, 0), consumedKeys.size());
    for (int i = 0; i < limit; ++i) {
        bool matched = false;
        consumedKeys.accessConst(i, [&matched, idempotencyKey](const uint64_t& value) {
            matched = (value == idempotencyKey);
        });
        if (matched) {
            return true;
        }
    }
    return false;
}

} // namespace hider_seeker