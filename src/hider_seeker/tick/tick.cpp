#include "tick.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../ruleset/ring_buffers.hpp"

#include <algorithm>

namespace hider_seeker {

TickOutput runTick(MatchState& state,
                    const Array<TickIntent, MAX_INTENTS_PER_TICK>& candidateIntents,
                    uint8_t candidateIntentCount,
                    const ConsumedKeySet& alreadyConsumed,
                    const iTargetRankingPolicy* seekerPolicy) {
    TickOutput output;
    
    // Step 1 (ELIGIBILITY): copy every eligible candidate's embedded
    // IntentRecord into eligibleRecords, with its candidateIntents index in the
    // parallel eligibleIndices array.
    Array<IntentRecord, MAX_INTENTS_PER_TICK> eligibleRecords;
    Array<uint8_t, MAX_INTENTS_PER_TICK> eligibleIndices;
    uint8_t eligibleCount = 0;

    for (int i = 0; i < candidateIntentCount; ++i) {
        candidateIntents.accessConst(i, [&](const TickIntent& candidate) {
            if (!alreadyConsumed.contains(candidate.intent.idempotencyKey)) {
                eligibleRecords.assignValue(eligibleCount, candidate.intent);
                eligibleIndices.assignValue(eligibleCount, static_cast<uint8_t>(i));
                eligibleCount++;
            }
        });
    }

    // Step 2 (ORDERING): order the eligible records with orderAcceptedIntents.
    // It sorts every slot of the array, so the unused slots past eligibleCount
    // are filled with a maximal receiptSequence to keep them sorted after
    // every real record (zeroed slots would otherwise sort first).
    Array<IntentRecord, MAX_INTENTS_PER_TICK> orderedRecords = eligibleRecords;
    for (int i = eligibleCount; i < orderedRecords.size(); ++i) {
        orderedRecords.assignValue(i, IntentRecord{UINT64_MAX, 0, 0, HiderActionMode{}});
    }
    orderAcceptedIntents(orderedRecords);

    // Step 3 (ACTIONS): process ordered intents
    Array<IdempotencyKeyT, MAX_INTENTS_PER_TICK> consumedKeysInThisTick;
    uint8_t consumedKeyCountInThisTick = 0;

    // Eligible slots already matched to an ordered record, so each eligible
    // TickIntent is dispatched exactly once.
    Array<bool, MAX_INTENTS_PER_TICK> claimed;
    for (auto& slot : claimed) {
        slot = false;
    }
    uint8_t dispatchedCount = 0;

    for (int i = 0; i < orderedRecords.size() && dispatchedCount < eligibleCount; ++i) {
        const IntentRecord defaultRecord{};
        const IntentRecord& orderedRecord = orderedRecords.getOrDefault(i, defaultRecord);

        // Map the ordered record back to the first unclaimed eligible
        // candidate (in candidate order) whose embedded record is identical.
        int matchedSlot = -1;
        for (int j = 0; j < eligibleCount; ++j) {
            if (claimed.getOrDefault(j, true)) {
                continue;
            }
            const IntentRecord& candidateRecord = eligibleRecords.getOrDefault(j, defaultRecord);
            if (candidateRecord.receiptSequence == orderedRecord.receiptSequence &&
                candidateRecord.idempotencyKey == orderedRecord.idempotencyKey &&
                candidateRecord.hiderId == orderedRecord.hiderId &&
                candidateRecord.mode == orderedRecord.mode) {
                matchedSlot = j;
                break;
            }
        }

        if (matchedSlot < 0) {
            continue; // padding slot: no eligible candidate behind it
        }
        claimed.assignValue(matchedSlot, true);
        dispatchedCount++;

        const uint8_t candidateIndex = eligibleIndices.getOrDefault(matchedSlot, 0);
        const TickIntent defaultIntent{};
        const TickIntent& matchingIntent = candidateIntents.getOrDefault(candidateIndex, defaultIntent);
        const IntentRecord& intentRecord = matchingIntent.intent;

        // Check if this intent was already applied in this tick (idempotency)
        // Using the existing isIntentAlreadyApplied function
        bool alreadyApplied = isIntentAlreadyApplied(consumedKeysInThisTick, consumedKeyCountInThisTick, intentRecord.idempotencyKey);
        
        if (alreadyApplied) {
            continue; // Skip this intent
        }
        
        RuleOutcome outcome;
        
        switch (matchingIntent.kind) {
            case TickActionKind::HIDER_ACTION:
                outcome = applyHiderAction(state, intentRecord.hiderId, matchingIntent.action_mode);
                break;
                
            case TickActionKind::ADD_SERVER:
                // For ADD_SERVER, we don't use target_server_id since it finds an available slot
                {
                    Array<PublicLifecycleEvent, MAX_EVENTS> dummyEvents;
                    uint8_t dummyCount = 0;
                    outcome = applyAddServer(state, intentRecord.hiderId, dummyEvents, dummyCount);
                }
                break;
                
            case TickActionKind::REMOVE_SERVER:
                // For REMOVE_SERVER, we need to pass the target_server_id
                {
                    Array<PublicLifecycleEvent, MAX_EVENTS> dummyEvents;
                    uint8_t dummyCount = 0;
                    outcome = applyRemoveServer(state, intentRecord.hiderId, matchingIntent.target_server_id, dummyEvents, dummyCount);
                }
                break;
        }
        
        // Record the outcome
        IntentOutcomeRecord record;
        record.idempotencyKey = intentRecord.idempotencyKey;
        record.outcome = outcome;
        output.intentOutcomes.assignValue(output.intentOutcomeCount, record);
        output.intentOutcomeCount++;

        // If accepted, add to consumed keys
        if (outcome == RuleOutcome::ACCEPTED) {
            output.consumedKeys.assignValue(output.consumedKeyCount, intentRecord.idempotencyKey);
            output.consumedKeyCount++;

            // Also track in this tick's consumed keys for idempotency checking
            consumedKeysInThisTick.assignValue(consumedKeyCountInThisTick, intentRecord.idempotencyKey);
            consumedKeyCountInThisTick++;
        }
    }
    
    // Step 4 (METRICS): push metric frames for active servers
    // PLACEHOLDER pending ruleset confirmation
    for (int i = 0; i < MAX_SERVERS; ++i) {
        if (state.servers[i].active) {
            MetricFrame frame = state.servers[i].current;
            if (state.servers[i].trace_strength > 0) {
                state.servers[i].trace_age++; // increment trace_age by 1 when trace_strength > 0
            }
            pushMetricFrame(state.servers[i], frame);
        }
    }
    
    // Step 6 (SEEKER-POLICY): resolve probes based on policy
    if (seekerPolicy != nullptr) {
        // Every slot starts as "no candidate" so slots the policy leaves
        // untouched are skipped below (a zeroed slot would mean server 0).
        Array<ProbeCandidate, MAX_SERVERS> candidates;
        for (auto& slot : candidates) {
            slot = ProbeCandidate{NO_SERVER_ID, 0};
        }
        seekerPolicy->rankTargets(state, candidates);
        
        // Step 7 (PROBES): process candidates in policy order within the token budget
        // Snapshot the initial connection_tokens balance to prevent mid-tick token awards
        // from allowing more applyProbe calls than permitted by the start-of-tick balance.
        // The snapshot is never refreshed inside the loop: tokens awarded by
        // resolveProbeConsequences during this tick do not raise the bound.
        const uint8_t initialTokens = state.seeker.connection_tokens;
        uint8_t probesIssued = 0;
        for (int i = 0; i < candidates.size(); ++i) {
            ProbeCandidate defaultCandidate{NO_SERVER_ID, 0};
            const ProbeCandidate& candidate = candidates.getOrDefault(i, defaultCandidate);

            // Skip invalid candidates (NO_SERVER_ID means no candidate)
            if (candidate.serverId == NO_SERVER_ID) {
                continue;
            }

            // Check against the initial token balance, not the current one
            if (probesIssued >= initialTokens) {
                break; // Start-of-probe-resolution budget exhausted
            }

            // A result applyProbe does not fill (rejected probe) is a miss against
            // the candidate's own server id, so resolveProbeConsequences never acts
            // on an unrelated server.
            ProbeResult result{};
            result.hit = false;
            result.targetServerId = candidate.serverId;

            // applyProbe deducts the per-probe token cost itself.
            applyProbe(state, candidate.serverId, result);
            probesIssued++;

            // Unconditionally resolve consequences for every policy-selected
            // candidate processed within the token budget (architecture plan).
            ProbeConsequences consequences = resolveProbeConsequences(state, result);

            output.probeResults.assignValue(output.probeCount, result);
            output.probeConsequences.assignValue(output.probeCount, consequences);
            output.probeCount++;
        }
    }
    
    return output;
}

} // namespace hider_seeker