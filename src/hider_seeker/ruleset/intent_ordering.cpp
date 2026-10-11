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

bool isIntentAlreadyApplied(const Array<uint64_t, MAX_INTENTS_PER_TICK>& consumedKeys, uint64_t idempotencyKey) {
    // Pure function that checks if an intent has already been applied
    // This module does not itself persist consumed keys since MatchState (frozen) 
    // has no field for them
    
    // Iterate through all elements in the array
    for (int i = 0; i < consumedKeys.size(); ++i) {
        // Use getOrDefault directly on the array - this is const-correct
        uint64_t value = consumedKeys.getOrDefault(i, 0);
        if (value == idempotencyKey) {
            return true;
        }
    }
    return false;
}

} // namespace hider_seeker