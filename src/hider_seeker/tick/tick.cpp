#include "tick.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../ruleset/ring_buffers.hpp"

namespace hider_seeker {

TickOutput runTick(MatchState& state,
                    const Array<TickIntent, MAX_INTENTS_PER_TICK>& candidateIntents,
                    uint8_t candidateIntentCount,
                    const ConsumedKeySet& alreadyConsumed,
                    const iTargetRankingPolicy* seekerPolicy) {
    TickOutput output;
    
    // Step 1 (ELIGIBILITY): filter eligible candidate intents
    Array<IntentRecord, MAX_INTENTS_PER_TICK> eligibleIntentRecords;
    uint8_t eligibleCount = 0;
    
    for (int i = 0; i < candidateIntentCount; ++i) {
        candidateIntents.accessConst(i, [&](const TickIntent& candidate) {
            if (!alreadyConsumed.contains(candidate.intent.idempotencyKey)) {
                eligibleIntentRecords.assignValue(eligibleCount, candidate.intent);
                eligibleCount++;
            }
        });
    }

    // Unused slots must sort after every eligible record so the eligible
    // records occupy [0, eligibleCount) after ordering.
    for (int i = eligibleCount; i < eligibleIntentRecords.size(); ++i) {
        eligibleIntentRecords.access(i, [](IntentRecord& unused) {
            unused.receiptSequence = UINT64_MAX;
        });
    }

    // Step 2 (ORDERING): order the eligible intents using the existing function
    orderAcceptedIntents(eligibleIntentRecords);
    
    // Step 3 (ACTIONS): process ordered intents
    Array<uint64_t, MAX_INTENTS_PER_TICK> consumedKeysInThisTick;
    uint8_t consumedKeyCountInThisTick = 0;
    
    for (int i = 0; i < eligibleCount; ++i) {
        // Find the original TickIntent that corresponds to this IntentRecord
        IntentRecord defaultRecord{};
        const IntentRecord& intentRecord = eligibleIntentRecords.getOrDefault(i, defaultRecord);
        const TickIntent* matchingIntent = nullptr;

        for (int j = 0; j < candidateIntentCount && matchingIntent == nullptr; ++j) {
            candidateIntents.accessConst(j, [&](const TickIntent& candidate) {
                if (candidate.intent.idempotencyKey == intentRecord.idempotencyKey) {
                    matchingIntent = &candidate;
                }
            });
        }
        
        if (!matchingIntent) {
            continue; // Should not happen, but just in case
        }
        
        // Check if this intent was already applied in this tick (idempotency)
        // Using the existing isIntentAlreadyApplied function
        bool alreadyApplied = isIntentAlreadyApplied(consumedKeysInThisTick, consumedKeyCountInThisTick, intentRecord.idempotencyKey);
        
        if (alreadyApplied) {
            continue; // Skip this intent
        }
        
        RuleOutcome outcome;
        
        switch (matchingIntent->kind) {
            case TickActionKind::HIDER_ACTION:
                outcome = applyHiderAction(state, intentRecord.hiderId, matchingIntent->action_mode);
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
                    outcome = applyRemoveServer(state, intentRecord.hiderId, matchingIntent->target_server_id, dummyEvents, dummyCount);
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
        for (int i = 0; i < candidates.size(); ++i) {
            ProbeCandidate defaultCandidate{NO_SERVER_ID, 0};
            const ProbeCandidate& candidate = candidates.getOrDefault(i, defaultCandidate);

            // Skip invalid candidates (NO_SERVER_ID means no candidate)
            if (candidate.serverId == NO_SERVER_ID) {
                continue;
            }

            // Before each candidate, re-read state.seeker.connection_tokens
            // (applyProbe spends tokens and resolveProbeConsequences may award them).
            if (state.seeker.connection_tokens == 0) {
                break; // No more tokens to spend
            }

            // A result applyProbe does not fill (rejected probe) is a miss against
            // the candidate's own server id, so resolveProbeConsequences never acts
            // on an unrelated server.
            ProbeResult result{};
            result.hit = false;
            result.targetServerId = candidate.serverId;

            // applyProbe deducts the per-probe token cost itself.
            applyProbe(state, candidate.serverId, result);

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