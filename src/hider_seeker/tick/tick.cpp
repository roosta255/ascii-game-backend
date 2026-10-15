#include "tick.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/loa_rules.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../ruleset/ring_buffers.hpp"
#include "../ruleset/server_metrics.hpp"

#include <algorithm>

namespace hider_seeker {

RuleOutcome applyTickIntent(MatchState& state, const TickIntent& intent,
                            Array<PublicLifecycleEvent, MAX_EVENTS>& outPublicEvents,
                            uint8_t& outPublicEventCount) {
    const IntentRecord& intentRecord = intent.intent;

    RuleOutcome outcome = RuleOutcome::REJECTED_INVALID_TARGET;

    switch (intent.kind) {
        case TickActionKind::HIDER_ACTION:
            outcome = applyHiderAction(state, intentRecord.hiderId, intentRecord.mode);
            break;

        case TickActionKind::ADD_SERVER:
            // For ADD_SERVER, we don't use target_server_id since it finds an available slot
            outcome = applyAddServer(state, intentRecord.hiderId, outPublicEvents, outPublicEventCount);
            break;

        case TickActionKind::REMOVE_SERVER:
            // For REMOVE_SERVER, we need to pass the target_server_id
            outcome = applyRemoveServer(state, intentRecord.hiderId, intent.target_server_id,
                                        outPublicEvents, outPublicEventCount);
            break;
    }

    return outcome;
}

RuleOutcome applyTickIntent(MatchState& state, const TickIntent& intent) {
    // Same single dispatch point; any public lifecycle event is discarded.
    Array<PublicLifecycleEvent, MAX_EVENTS> discardedEvents;
    uint8_t discardedEventCount = 0;
    return applyTickIntent(state, intent, discardedEvents, discardedEventCount);
}

namespace {

// An eligible candidate: its embedded record, where it sits in
// candidateIntents, and whether it has been dispatched yet this tick.
struct EligibleIntent {
    IntentRecord record;
    uint8_t candidateIndex;
    bool dispatched;
};

bool isSameRecord(const IntentRecord& a, const IntentRecord& b) {
    return a.receiptSequence == b.receiptSequence &&
           a.idempotencyKey == b.idempotencyKey &&
           a.hiderId == b.hiderId &&
           a.mode == b.mode;
}

void dispatchIntent(MatchState& state,
                    const TickIntent& matchingIntent,
                    Array<IdempotencyKeyT, MAX_INTENTS_PER_TICK>& consumedKeysInThisTick,
                    uint8_t& consumedKeyCountInThisTick,
                    TickOutput& output) {
    const IntentRecord& intentRecord = matchingIntent.intent;

    // Check if this intent was already applied in this tick (idempotency)
    // Using the existing isIntentAlreadyApplied function
    if (isIntentAlreadyApplied(consumedKeysInThisTick, consumedKeyCountInThisTick, intentRecord.idempotencyKey)) {
        return; // Skip this intent
    }

    RuleOutcome outcome = applyTickIntent(state, matchingIntent, output.publicEvents, output.publicEventCount);

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

} // namespace

TickOutput runTick(MatchState& state,
                    const Array<TickIntent, MAX_INTENTS_PER_TICK>& candidateIntents,
                    uint8_t candidateIntentCount,
                    const ConsumedKeySet& alreadyConsumed,
                    const iTargetRankingPolicy* seekerPolicy) {
    TickOutput output;
    
    // Step 1 (ELIGIBILITY): collect every candidate whose key isn't already
    // consumed, with its candidateIntents index, and its embedded
    // IntentRecord for ordering.
    Array<EligibleIntent, MAX_INTENTS_PER_TICK> eligible;
    Array<IntentRecord, MAX_INTENTS_PER_TICK> orderedRecords;
    uint8_t eligibleCount = 0;

    for (int i = 0; i < candidateIntentCount; ++i) {
        candidateIntents.accessConst(i, [&](const TickIntent& candidate) {
            if (!alreadyConsumed.contains(candidate.intent.idempotencyKey)) {
                eligible.assignValue(eligibleCount, EligibleIntent{candidate.intent, static_cast<uint8_t>(i), false});
                orderedRecords.assignValue(eligibleCount, candidate.intent);
                eligibleCount++;
            }
        });
    }

    // Step 2 (ORDERING): order the eligible records with orderAcceptedIntents.
    // It sorts every slot of the array, so the unused slots past eligibleCount
    // are filled with a maximal receiptSequence to keep them sorted after
    // every real record (zeroed slots would otherwise sort first).
    for (int i = eligibleCount; i < orderedRecords.size(); ++i) {
        orderedRecords.assignValue(i, IntentRecord{UINT64_MAX, 0, 0, HiderActionMode{}});
    }
    orderAcceptedIntents(orderedRecords);

    // Step 3 (ACTIONS): dispatch the eligible intents in that order.
    Array<IdempotencyKeyT, MAX_INTENTS_PER_TICK> consumedKeysInThisTick;
    uint8_t consumedKeyCountInThisTick = 0;
    uint8_t dispatchedCount = 0;

    for (const IntentRecord& orderedRecord : orderedRecords) {
        if (dispatchedCount == eligibleCount) {
            break; // only padding is left
        }

        // Map the ordered record back to the first not-yet-dispatched
        // eligible candidate (in candidate order) with an identical record,
        // so each eligible TickIntent is dispatched exactly once.
        uint8_t slotsSeen = 0;
        for (EligibleIntent& slot : eligible) {
            if (slotsSeen++ == eligibleCount) {
                break;
            }
            if (slot.dispatched || !isSameRecord(slot.record, orderedRecord)) {
                continue;
            }
            slot.dispatched = true;
            dispatchedCount++;
            candidateIntents.accessConst(slot.candidateIndex, [&](const TickIntent& matchingIntent) {
                dispatchIntent(state, matchingIntent, consumedKeysInThisTick, consumedKeyCountInThisTick, output);
            });
            break;
        }
    }
    
    // Step 4 (METRICS): for each active server, drift every metric toward its
    // rest value (in ServerMetric order), then age a still-present trace
    // (evaluated after that decay), then push the frame. Inactive servers are
    // left untouched.
    for (ServerState& server : state.servers) {
        if (!server.active) {
            continue;
        }
        // The index is the ServerMetric being drifted.
        for (int m = 0; m < SERVER_METRIC_COUNT; ++m) {
            const ServerMetric metric = static_cast<ServerMetric>(m);
            driftMetric(server, metric, serverMetricDef(metric));
        }
        if (server.trace_strength > 0) {
            server.trace_age++;
        }
        pushMetricFrame(server, server.current);
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
        for (const ProbeCandidate& candidate : candidates) {
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