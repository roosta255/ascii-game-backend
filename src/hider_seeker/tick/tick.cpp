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
        if (!alreadyConsumed.contains(candidateIntents[i].intent.idempotencyKey)) {
            eligibleIntentRecords[eligibleCount] = candidateIntents[i].intent;
            eligibleCount++;
        }
    }
    
    // Step 2 (ORDERING): order the eligible intents using the existing function
    orderAcceptedIntents(eligibleIntentRecords);
    
    // Step 3 (ACTIONS): process ordered intents
    Array<uint64_t, MAX_INTENTS_PER_TICK> consumedKeysInThisTick;
    uint8_t consumedKeyCountInThisTick = 0;
    
    for (int i = 0; i < eligibleCount; ++i) {
        // Find the original TickIntent that corresponds to this IntentRecord
        const IntentRecord& intentRecord = eligibleIntentRecords[i];
        TickIntent* matchingIntent = nullptr;
        
        for (int j = 0; j < candidateIntentCount; ++j) {
            if (candidateIntents[j].intent.idempotencyKey == intentRecord.idempotencyKey) {
                matchingIntent = &candidateIntents[j];
                break;
            }
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
                // For ADD_SERVER, we need to pass the target_server_id
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
        output.intentOutcomes[output.intentOutcomeCount] = record;
        output.intentOutcomeCount++;
        
        // If accepted, add to consumed keys
        if (outcome == RuleOutcome::ACCEPTED) {
            output.consumedKeys[output.consumedKeyCount] = intentRecord.idempotencyKey;
            output.consumedKeyCount++;
            
            // Also track in this tick's consumed keys for idempotency checking
            consumedKeysInThisTick[consumedKeyCountInThisTick] = intentRecord.idempotencyKey;
            consumedKeyCountInThisTick++;
        }
    }
    
    // Step 4 (METRICS): push metric frames for active servers
    // PLACEHOLDER pending ruleset confirmation
    for (int i = 0; i < MAX_SERVERS; ++i) {
        if (state.servers[i].active) {
            MetricFrame frame = state.servers[i].current;
            if (frame.errors > 0) {
                frame.errors++; // increment trace_age by 1 when trace_strength > 0
            }
            pushMetricFrame(state.servers[i], frame);
        }
    }
    
    // Step 6 (SEEKER-POLICY): resolve probes based on policy
    if (seekerPolicy != nullptr) {
        Array<ProbeCandidate, MAX_SERVERS> candidates;
        seekerPolicy->rankTargets(state, candidates);
        
        // Process up to connection_tokens number of probes
        int tokens = state.seeker.connection_tokens;
        for (int i = 0; i < candidates.size() && i < tokens; ++i) {
            ProbeResult result;
            RuleOutcome probeOutcome = applyProbe(state, candidates[i].serverId, result);
            
            if (probeOutcome == RuleOutcome::ACCEPTED) {
                ProbeConsequences consequences = resolveProbeConsequences(state, result);
                
                output.probeResults[output.probeCount] = result;
                output.probeConsequences[output.probeCount] = consequences;
                output.probeCount++;
            }
        }
    }
    
    return output;
}

} // namespace hider_seeker