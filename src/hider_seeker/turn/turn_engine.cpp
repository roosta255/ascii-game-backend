#include "turn_engine.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../ruleset/regeneration.hpp"
#include "../tick/tick.hpp"
#include "../ruleset/connection_rules.hpp"

namespace hider_seeker {

namespace {

// runTick is always called with an empty consumed-key set: pending moves are
// removed from the ring once applied, so they are never resubmitted.
const ConsumedKeySet NO_CONSUMED_KEYS{};

TurnResult rejected(TurnReason reason, RuleOutcome ruleOutcome = RuleOutcome::ACCEPTED) {
    TurnResult result{};
    result.status = MoveStatus::REJECTED;
    result.reason = reason;
    result.ruleOutcome = ruleOutcome;
    return result;
}

bool isCaptured(const MatchState& state, uint8_t hiderId) {
    return hiderId < MAX_HIDERS && state.hiders[hiderId].status == HIDER_STATUS_CAPTURED;
}

// TURN-008 common checks for an entry point acting on behalf of hiderId.
// On failure, fills out with the REJECTED result and returns false.
bool passesHiderChecks(const MatchState& state, uint8_t hiderId, TurnResult& out) {
    if (state.phase != PHASE_ACTIVE) {
        out = rejected(TurnReason::MATCH_NOT_ACTIVE);
        return false;
    }
    if (hiderId >= MAX_HIDERS) {
        out = rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
        return false;
    }
    if (isCaptured(state, hiderId)) {
        out = rejected(TurnReason::HIDER_CAPTURED);
        return false;
    }
    if (!state.hiders[hiderId].active) {
        out = rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
        return false;
    }
    return true;
}

// Common checks plus move well-formedness: action_type must name a
// TickActionKind and, for HIDER_ACTION, server_mode a HiderActionMode (the
// action rule table is indexed by it).
bool passesMoveChecks(const MatchState& state, const MoveRequest& move, TurnResult& out) {
    if (!passesHiderChecks(state, move.hider_id, out)) {
        return false;
    }
    const bool knownKind = move.action_type <= static_cast<uint8_t>(TickActionKind::DISCONNECT);
    const bool knownMode = move.action_type != static_cast<uint8_t>(TickActionKind::HIDER_ACTION) ||
                           move.server_mode < HIDER_ACTION_MODE_COUNT;
    if (!knownKind || !knownMode) {
        out = rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
        return false;
    }
    return true;
}

TickIntent makeIntent(uint32_t moveId, uint64_t receiptSequence, uint8_t hiderId, uint8_t actionType,
                      uint8_t serverId, uint8_t serverMode) {
    TickIntent intent{};
    intent.kind = static_cast<TickActionKind>(actionType);
    intent.target_server_id = serverId;
    intent.intent.receiptSequence = receiptSequence;
    intent.intent.idempotencyKey = moveId;
    intent.intent.hiderId = hiderId;
    intent.intent.mode = static_cast<HiderActionMode>(serverMode);
    return intent;
}

TickIntent intentFromMove(const MoveRequest& move, uint64_t receiptSequence) {
    return makeIntent(move.move_id, receiptSequence, move.hider_id, move.action_type, move.server_id,
                      move.server_mode);
}

TickIntent intentFromPending(const PendingMove& pending) {
    return makeIntent(pending.move_id, pending.submit_seq, pending.hider_id, pending.action_type,
                      pending.server_id, pending.server_mode);
}

// Outcome of intent against a disposable copy of state; state is untouched.
RuleOutcome trialOutcome(const MatchState& state, const TickIntent& intent) {
    MatchState scratch = state;
    return applyTickIntent(scratch, intent);
}

// TURN-004 pre-check: whether regeneration alone makes intent affordable
// within MAX_ADVANCE_TICKS ticks. Works on copies only and never calls the
// seeker policy or runTick.
bool becomesAffordable(const MatchState& state, const TickIntent& intent) {
    MatchState projected = state;
    for (uint8_t tick = 0; tick < MAX_ADVANCE_TICKS; ++tick) {
        regenerateForTick(projected);
        if (trialOutcome(projected, intent) == RuleOutcome::ACCEPTED) {
            return true;
        }
    }
    return false;
}

// TURN-003: apply an affordable move right now, with no tick advanced.
void applyNow(MatchState& state, const TickIntent& intent, TurnResult& result) {
    result.ruleOutcome = applyTickIntent(state, intent, result.publicEvents, result.publicEventCount);
    result.status = MoveStatus::APPLIED;
    result.reason = TurnReason::NONE;
    result.ticksAdvanced = 0;
    state.revision += 1;
}

// Pending ring helpers. Ring positions are offsets from pending_head, in
// submit_seq order.
uint8_t ringIndex(const MatchState& state, uint8_t offset) {
    return static_cast<uint8_t>((state.pending_head + offset) % MAX_PENDING_MOVES);
}

uint8_t pendingCountForHider(const MatchState& state, uint8_t hiderId) {
    uint8_t count = 0;
    for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
        if (state.pending[ringIndex(state, offset)].hider_id == hiderId) {
            count++;
        }
    }
    return count;
}

bool hasPendingMoveId(const MatchState& state, uint32_t moveId) {
    for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
        if (state.pending[ringIndex(state, offset)].move_id == moveId) {
            return true;
        }
    }
    return false;
}

// Removes every pending move flagged in removeAtOffset (indexed by ring
// offset), keeps the rest in their original order starting at pending_head,
// and clears the freed slots.
void compactPending(MatchState& state, const bool (&removeAtOffset)[MAX_PENDING_MOVES]) {
    PendingMove kept[MAX_PENDING_MOVES] = {};
    uint8_t keptCount = 0;
    for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
        if (!removeAtOffset[offset]) {
            kept[keptCount++] = state.pending[ringIndex(state, offset)];
        }
    }
    for (uint8_t offset = 0; offset < MAX_PENDING_MOVES; ++offset) {
        state.pending[ringIndex(state, offset)] = offset < keptCount ? kept[offset] : PendingMove{};
    }
    state.pending_count = keptCount;
}

// The rule outcome runTick recorded for moveId's idempotency key. A
// candidate with no recorded outcome is treated as not yet affordable, so it
// stays waiting.
RuleOutcome outcomeFor(const TickOutput& output, uint32_t moveId) {
    RuleOutcome found = RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    bool matched = false;
    for (int i = 0; i < output.intentOutcomeCount && !matched; ++i) {
        output.intentOutcomes.accessConst(i, [&](const IntentOutcomeRecord& record) {
            if (record.idempotencyKey == moveId) {
                found = record.outcome;
                matched = true;
            }
        });
    }
    return found;
}

void appendPublicEvents(TurnResult& result, const TickOutput& output) {
    for (int i = 0; i < output.publicEventCount; ++i) {
        output.publicEvents.accessConst(i, [&result](const PublicLifecycleEvent& event) {
            if (result.publicEvents.containsIndex(result.publicEventCount)) {
                result.publicEvents.assignValue(result.publicEventCount, event);
                result.publicEventCount++;
            }
        });
    }
}

// TURN-006 shared advance loop. Runs up to tickBudget ticks. With a
// triggering move, stops as soon as that move is applied or cancelled;
// without one (passTicks), runs the whole budget. Either way it stops early
// when the match completes. Ticks that ran stay applied. Does not touch
// state.revision (the caller bumps it once).
void runAdvanceLoop(MatchState& state, iTargetRankingPolicy& seekerPolicy, uint8_t tickBudget,
                    bool hasTrigger, const MoveRequest& trigger, TurnResult& result) {
    // Outcome if the budget runs out.
    result.status = hasTrigger ? MoveStatus::CANCELLED : MoveStatus::APPLIED;
    result.reason = hasTrigger ? TurnReason::ADVANCE_LIMIT : TurnReason::NONE;
    result.ruleOutcome = hasTrigger ? RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE : RuleOutcome::ACCEPTED;

    while (result.ticksAdvanced < tickBudget) {
        state.tick += 1;
        result.ticksAdvanced++;
        regenerateForTick(state);

        // Candidates: every pending move in ring (submit_seq) order, then the
        // triggering move as the newest submission.
        Array<TickIntent, MAX_INTENTS_PER_TICK> candidates;
        uint8_t candidateCount = 0;
        for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
            candidates.assignValue(candidateCount++, intentFromPending(state.pending[ringIndex(state, offset)]));
        }
        if (hasTrigger) {
            candidates.assignValue(candidateCount++, intentFromMove(trigger, state.next_submit_seq));
        }

        const TickOutput output = runTick(state, candidates, candidateCount, NO_CONSUMED_KEYS, &seekerPolicy);
        appendPublicEvents(result, output);
        result.probesIssued += output.probeCount;

        // Pending moves: applied ones leave the ring and are reported;
        // rule-rejected ones are cancelled (RULE_REJECTED); still-unaffordable
        // ones stay unless their hider is now captured (HIDER_CAPTURED).
        bool removeAtOffset[MAX_PENDING_MOVES] = {};
        for (uint8_t offset = 0; offset < state.pending_count; ++offset) {
            const PendingMove& pending = state.pending[ringIndex(state, offset)];
            const RuleOutcome outcome = outcomeFor(output, pending.move_id);
            if (outcome == RuleOutcome::ACCEPTED) {
                removeAtOffset[offset] = true;
                if (result.pendingApplied.containsIndex(result.pendingAppliedCount)) {
                    result.pendingApplied.assignValue(result.pendingAppliedCount,
                                                      PendingMoveApplied{pending.move_id, pending.hider_id, state.tick});
                    result.pendingAppliedCount++;
                }
            } else if (outcome != RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
                removeAtOffset[offset] = true; // cancelled: RULE_REJECTED
            } else if (isCaptured(state, pending.hider_id)) {
                removeAtOffset[offset] = true; // cancelled: HIDER_CAPTURED
            }
        }
        compactPending(state, removeAtOffset);

        if (hasTrigger) {
            const RuleOutcome outcome = outcomeFor(output, trigger.move_id);
            result.ruleOutcome = outcome;
            if (outcome == RuleOutcome::ACCEPTED) {
                result.status = MoveStatus::APPLIED;
                result.reason = TurnReason::NONE;
                return;
            }
            if (outcome != RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
                result.status = MoveStatus::CANCELLED;
                result.reason = TurnReason::RULE_REJECTED;
                return;
            }
            if (isCaptured(state, trigger.hider_id)) {
                result.status = MoveStatus::CANCELLED;
                result.reason = TurnReason::HIDER_CAPTURED;
                return;
            }
        }

        if (state.phase == PHASE_COMPLETE) {
            if (hasTrigger) {
                result.status = MoveStatus::CANCELLED;
                result.reason = TurnReason::MATCH_NOT_ACTIVE;
            }
            return;
        }
    }
}

} // namespace

TurnResult playMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy) {
    TurnResult result{};
    if (!passesMoveChecks(state, move, result)) {
        return result;
    }

    // Special handling for CONNECT/DISCONNECT actions - they require the hider to be connected
    if (move.action_type == static_cast<uint8_t>(TickActionKind::CONNECT) || 
        move.action_type == static_cast<uint8_t>(TickActionKind::DISCONNECT)) {
        const uint8_t hiderId = move.hider_id;
        if (hiderId >= MAX_HIDERS || !state.hiders[hiderId].active) {
            return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
        }
        // Check if hider is connected
        if (move.action_type == static_cast<uint8_t>(TickActionKind::CONNECT)) {
            // For CONNECT, we need to check that the hider is not already connected
            if (state.hiders[hiderId].server_id != NO_SERVER_ID) {
                return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
            }
        } else if (move.action_type == static_cast<uint8_t>(TickActionKind::DISCONNECT)) {
            // For DISCONNECT, we need to check that the hider is connected
            if (state.hiders[hiderId].server_id == NO_SERVER_ID) {
                return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
            }
        }
    }

    const TickIntent intent = intentFromMove(move, state.next_submit_seq);
    const RuleOutcome nowOutcome = trialOutcome(state, intent);
    if (nowOutcome == RuleOutcome::ACCEPTED) {
        applyNow(state, intent, result);
        return result;
    }
    if (nowOutcome != RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
        return rejected(TurnReason::RULE_REJECTED, nowOutcome);
    }
    if (!becomesAffordable(state, intent)) {
        return rejected(TurnReason::NEVER_AFFORDABLE, RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    }

    runAdvanceLoop(state, seekerPolicy, MAX_ADVANCE_TICKS, true, move, result);
    state.revision += 1;
    return result;
}

TurnResult queueMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy) {
    (void)seekerPolicy; // queueMove never advances a tick, so the policy is never consulted
    TurnResult result{};
    if (!passesMoveChecks(state, move, result)) {
        return result;
    }

    // Special handling for CONNECT/DISCONNECT actions - they require the hider to be connected
    if (move.action_type == static_cast<uint8_t>(TickActionKind::CONNECT) || 
        move.action_type == static_cast<uint8_t>(TickActionKind::DISCONNECT)) {
        const uint8_t hiderId = move.hider_id;
        if (hiderId >= MAX_HIDERS || !state.hiders[hiderId].active) {
            return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
        }
        // Check if hider is connected
        if (move.action_type == static_cast<uint8_t>(TickActionKind::CONNECT)) {
            // For CONNECT, we need to check that the hider is not already connected
            if (state.hiders[hiderId].server_id != NO_SERVER_ID) {
                return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
            }
        } else if (move.action_type == static_cast<uint8_t>(TickActionKind::DISCONNECT)) {
            // For DISCONNECT, we need to check that the hider is connected
            if (state.hiders[hiderId].server_id == NO_SERVER_ID) {
                return rejected(TurnReason::RULE_REJECTED, RuleOutcome::REJECTED_INVALID_TARGET);
            }
        }
    }

    const TickIntent intent = intentFromMove(move, state.next_submit_seq);
    const RuleOutcome nowOutcome = trialOutcome(state, intent);
    if (nowOutcome == RuleOutcome::ACCEPTED) {
        applyNow(state, intent, result);
        return result;
    }
    if (nowOutcome != RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
        return rejected(TurnReason::RULE_REJECTED, nowOutcome);
    }

    // Queue-specific checks, in this fixed order.
    if (state.pending_count >= MAX_PENDING_MOVES) {
        return rejected(TurnReason::QUEUE_FULL, nowOutcome);
    }
    if (pendingCountForHider(state, move.hider_id) >= PLACEHOLDER_MAX_PENDING_PER_HIDER) {
        return rejected(TurnReason::HIDER_QUEUE_LIMIT, nowOutcome);
    }
    if (hasPendingMoveId(state, move.move_id)) {
        return rejected(TurnReason::DUPLICATE_MOVE_ID, nowOutcome);
    }

    state.pending[ringIndex(state, state.pending_count)] = PendingMove{
        move.move_id, state.next_submit_seq, move.hider_id, move.action_type,
        move.server_id, move.target_id, move.server_mode};
    state.pending_count++;
    state.next_submit_seq++;
    state.revision += 1;

    result.status = MoveStatus::QUEUED;
    result.reason = TurnReason::NONE;
    result.ruleOutcome = nowOutcome;
    return result;
}

TurnResult passTicks(MatchState& state, uint8_t ticks, iTargetRankingPolicy& seekerPolicy) {
    if (state.phase != PHASE_ACTIVE) {
        return rejected(TurnReason::MATCH_NOT_ACTIVE);
    }
    if (ticks == 0 || ticks > MAX_ADVANCE_TICKS) {
        return rejected(TurnReason::INVALID_TICK_COUNT);
    }

    TurnResult result{};
    const MoveRequest noTrigger{};
    runAdvanceLoop(state, seekerPolicy, ticks, false, noTrigger, result);
    state.revision += 1;
    return result;
}

TurnResult cancelPendingMove(MatchState& state, uint8_t hiderId, uint32_t moveId) {
    TurnResult result{};
    if (!passesHiderChecks(state, hiderId, result)) {
        return result;
    }

    bool removeAtOffset[MAX_PENDING_MOVES] = {};
    bool found = false;
    for (uint8_t offset = 0; offset < state.pending_count && !found; ++offset) {
        const PendingMove& pending = state.pending[ringIndex(state, offset)];
        if (pending.hider_id == hiderId && pending.move_id == moveId) {
            removeAtOffset[offset] = true;
            found = true;
        }
    }
    if (!found) {
        return rejected(TurnReason::NOT_FOUND);
    }

    compactPending(state, removeAtOffset);
    state.revision += 1;

    result.status = MoveStatus::CANCELLED;
    result.reason = TurnReason::NONE;
    return result;
}

} // namespace hider_seeker
