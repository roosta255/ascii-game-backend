#include "turn_engine.hpp"
#include "../hider_seeker/ruleset/regeneration.hpp"
#include "../hider_seeker/tick/tick.hpp"
#include "../hider_seeker/domain/state.hpp"

namespace hider_seeker {

namespace {
    // Helper to check if a hider is valid for making moves
    bool isValidHider(const MatchState& state, uint8_t hiderId) {
        return (hiderId < MAX_HIDERS && 
                state.hiders[hiderId].active && 
                state.hiders[hiderId].status != HIDER_STATUS_CAPTURED);
    }
    
    // Helper to check if a move is affordable immediately
    bool isMoveAffordable(MatchState& state, const MoveRequest& move) {
        // Create a temporary TickIntent from the move request
        TickIntent intent;
        intent.kind = static_cast<TickActionKind>(move.action_type);
        intent.target_server_id = move.server_id;
        
        // Create a dummy IntentRecord for the intent
        intent.intent.idempotencyKey = move.move_id;
        intent.intent.receiptSequence = 0; // This will be set properly later
        intent.intent.hiderId = move.hider_id;
        intent.intent.mode = static_cast<HiderActionMode>(move.server_mode);
        
        // Test the intent against the current state
        return applyTickIntent(state, intent) == RuleOutcome::ACCEPTED;
    }
    
    // Helper to find a pending move by hider and move ID
    int findPendingMove(const MatchState& state, uint8_t hiderId, uint32_t moveId) {
        for (int i = 0; i < state.pending_count; ++i) {
            int index = (state.pending_head + i) % MAX_PENDING_MOVES;
            if (state.pending[index].hider_id == hiderId && 
                state.pending[index].move_id == moveId) {
                return index;
            }
        }
        return -1; // Not found
    }
    
    // Helper to check if a hider has too many pending moves
    bool hasTooManyPendingMoves(const MatchState& state, uint8_t hiderId) {
        int count = 0;
        for (int i = 0; i < state.pending_count; ++i) {
            int index = (state.pending_head + i) % MAX_PENDING_MOVES;
            if (state.pending[index].hider_id == hiderId) {
                count++;
            }
        }
        return count >= PLACEHOLDER_MAX_PENDING_PER_HIDER;
    }
    
    // Helper to check if a move ID already exists in pending moves
    bool hasDuplicateMoveId(const MatchState& state, uint32_t moveId) {
        for (int i = 0; i < state.pending_count; ++i) {
            int index = (state.pending_head + i) % MAX_PENDING_MOVES;
            if (state.pending[index].move_id == moveId) {
                return true;
            }
        }
        return false;
    }
    
    // Helper to advance the tick loop
    TurnResult advanceLoop(MatchState& state, const MoveRequest* triggeringMove, 
                          iTargetRankingPolicy& seekerPolicy, uint8_t ticksToAdvance) {
        TurnResult result{};
        result.ticksAdvanced = 0;
        result.status = MoveStatus::APPLIED;
        result.reason = TurnReason::NONE;
        
        // If no ticks to advance, return immediately
        if (ticksToAdvance == 0 || state.phase != PHASE_ACTIVE) {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::INVALID_TICK_COUNT;
            return result;
        }
        
        // Loop for the specified number of ticks or until completion
        for (uint8_t tick = 0; tick < ticksToAdvance && state.phase == PHASE_ACTIVE; ++tick) {
            // Increment tick
            state.tick++;
            
            // Regenerate resources
            regenerateForTick(state);
            
            // Build candidate list from pending moves and triggering move if provided
            Array<TickIntent, MAX_PENDING_MOVES + 1> candidates;
            uint8_t candidateCount = 0;
            
            // Add all pending moves in ring order
            for (int i = 0; i < state.pending_count; ++i) {
                int index = (state.pending_head + i) % MAX_PENDING_MOVES;
                const PendingMove& pending = state.pending[index];
                
                if (pending.move_id == 0) continue; // Skip empty slots
                
                TickIntent intent;
                intent.kind = static_cast<TickActionKind>(pending.action_type);
                intent.target_server_id = pending.server_id;
                
                intent.intent.idempotencyKey = pending.move_id;
                intent.intent.receiptSequence = pending.submit_seq;
                intent.intent.hiderId = pending.hider_id;
                intent.intent.mode = static_cast<HiderActionMode>(pending.server_mode);
                
                candidates.assignValue(candidateCount, intent);
                candidateCount++;
            }
            
            // Add triggering move if provided
            if (triggeringMove != nullptr) {
                TickIntent intent;
                intent.kind = static_cast<TickActionKind>(triggeringMove->action_type);
                intent.target_server_id = triggeringMove->server_id;
                
                intent.intent.idempotencyKey = triggeringMove->move_id;
                intent.intent.receiptSequence = state.next_submit_seq;
                intent.intent.hiderId = triggeringMove->hider_id;
                intent.intent.mode = static_cast<HiderActionMode>(triggeringMove->server_mode);
                
                candidates.assignValue(candidateCount, intent);
                candidateCount++;
            }
            
            // Run the tick
            ConsumedKeySet alreadyConsumed;
            TickOutput output = runTick(state, candidates, candidateCount, alreadyConsumed, &seekerPolicy);
            
            // Process results and update pending moves
            for (int i = 0; i < output.intentOutcomeCount; ++i) {
                const IntentOutcomeRecord& record = output.intentOutcomes.getOrDefault(i, IntentOutcomeRecord{});
                
                // Find the corresponding pending move or triggering move
                bool found = false;
                int pendingIndex = -1;
                
                // Check if this outcome corresponds to a pending move
                for (int j = 0; j < state.pending_count; ++j) {
                    int index = (state.pending_head + j) % MAX_PENDING_MOVES;
                    if (state.pending[index].move_id == record.idempotencyKey) {
                        pendingIndex = index;
                        found = true;
                        break;
                    }
                }
                
                // If it's a triggering move, we need to handle it differently
                if (!found && triggeringMove != nullptr && triggeringMove->move_id == record.idempotencyKey) {
                    // This is the triggering move - we'll process it separately
                } else if (found) {
                    // Process the outcome for a pending move
                    if (record.outcome == RuleOutcome::ACCEPTED) {
                        // Remove from pending moves and add to applied list
                        PendingMoveApplied applied;
                        applied.move_id = record.idempotencyKey;
                        applied.hider_id = state.pending[pendingIndex].hider_id;
                        applied.tick = state.tick;
                        
                        result.pendingApplied.assignValue(result.pendingAppliedCount, applied);
                        result.pendingAppliedCount++;
                        
                        // Remove the pending move
                        state.pending[pendingIndex] = PendingMove{};
                        state.pending_count--;
                    } else if (record.outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
                        // Keep in pending moves - not enough resources yet
                    } else {
                        // Cancel the move with reason RULE_REJECTED
                        // We don't remove it from pending, but we'll mark it as cancelled
                        // For simplicity, we'll just remove it from the ring
                        state.pending[pendingIndex] = PendingMove{};
                        state.pending_count--;
                    }
                } else if (triggeringMove != nullptr && triggeringMove->move_id == record.idempotencyKey) {
                    // Handle the triggering move outcome
                    if (record.outcome == RuleOutcome::ACCEPTED) {
                        // Triggering move was applied - we don't need to do anything special here
                        result.status = MoveStatus::APPLIED;
                    } else if (record.outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE) {
                        // Keep the triggering move in pending for next tick
                        result.status = MoveStatus::QUEUED;
                    } else {
                        // Cancelled due to rule rejection
                        result.status = MoveStatus::CANCELLED;
                        result.reason = TurnReason::RULE_REJECTED;
                    }
                }
            }
            
            // Append public events and probe count to result
            for (int i = 0; i < output.publicEventCount; ++i) {
                const PublicLifecycleEvent& event = output.publicEvents.getOrDefault(i, PublicLifecycleEvent{});
                result.publicEvents.assignValue(result.publicEventCount, event);
                result.publicEventCount++;
            }
            
            result.probesIssued += output.probeCount;
            result.ticksAdvanced++;
            
            // Compact the pending ring after each tick
            int writeIndex = 0;
            for (int i = 0; i < state.pending_count; ++i) {
                int readIndex = (state.pending_head + i) % MAX_PENDING_MOVES;
                if (state.pending[readIndex].move_id != 0) { // Only copy non-empty moves
                    if (writeIndex != i) {
                        state.pending[(state.pending_head + writeIndex) % MAX_PENDING_MOVES] = 
                            state.pending[readIndex];
                    }
                    writeIndex++;
                }
            }
            
            // Update head and count to reflect compaction
            state.pending_count = writeIndex;
            if (writeIndex > 0) {
                state.pending_head = (state.pending_head + 0) % MAX_PENDING_MOVES; // No change in head since we're compacting from the front
            } else {
                state.pending_head = 0;
            }
            
            // Check if any hider has been captured during this tick
            for (int i = 0; i < MAX_HIDERS; ++i) {
                if (state.hiders[i].status == HIDER_STATUS_CAPTURED) {
                    // Cancel all pending moves for this hider
                    int j = 0;
                    while (j < state.pending_count) {
                        int index = (state.pending_head + j) % MAX_PENDING_MOVES;
                        if (state.pending[index].hider_id == i) {
                            // Mark as cancelled and remove
                            state.pending[index] = PendingMove{};
                            state.pending_count--;
                        } else {
                            j++;
                        }
                    }
                }
            }
            
            // Check if the triggering move's hider was captured
            if (triggeringMove != nullptr && 
                state.hiders[triggeringMove->hider_id].status == HIDER_STATUS_CAPTURED) {
                result.status = MoveStatus::CANCELLED;
                result.reason = TurnReason::HIDER_CAPTURED;
                break;
            }
            
            // Check if the match is complete
            if (state.phase == PHASE_COMPLETE) {
                result.status = MoveStatus::CANCELLED;
                result.reason = TurnReason::MATCH_NOT_ACTIVE;
                break;
            }
        }
        
        return result;
    }
} // namespace

TurnResult playMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy) {
    TurnResult result{};
    
    // Common checks
    if (state.phase != PHASE_ACTIVE) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::MATCH_NOT_ACTIVE;
        return result;
    }
    
    if (!isValidHider(state, move.hider_id)) {
        if (state.hiders[move.hider_id].status == HIDER_STATUS_CAPTURED) {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::HIDER_CAPTURED;
        } else {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::RULE_REJECTED;
            result.ruleOutcome = RuleOutcome::REJECTED_INVALID_TARGET;
        }
        return result;
    }
    
    // Immediate apply path - if affordable, apply immediately
    if (isMoveAffordable(state, move)) {
        // Create the intent and apply it directly to the real state
        TickIntent intent;
        intent.kind = static_cast<TickActionKind>(move.action_type);
        intent.target_server_id = move.server_id;
        
        intent.intent.idempotencyKey = move.move_id;
        intent.intent.receiptSequence = 0; // Not used for immediate apply
        intent.intent.hiderId = move.hider_id;
        intent.intent.mode = static_cast<HiderActionMode>(move.server_mode);
        
        // Apply the tick intent directly to the state
        RuleOutcome outcome = applyTickIntent(state, intent);
        
        result.status = MoveStatus::APPLIED;
        result.ticksAdvanced = 0;
        result.ruleOutcome = outcome;
        state.revision++;
        return result;
    }
    
    // Never affordable pre-check - check if it can be affordable within MAX_ADVANCE_TICKS
    MatchState tempState = state; // Copy the state for testing
    bool affordable = false;
    
    for (int i = 0; i < MAX_ADVANCE_TICKS; ++i) {
        regenerateForTick(tempState);
        
        TickIntent intent;
        intent.kind = static_cast<TickActionKind>(move.action_type);
        intent.target_server_id = move.server_id;
        
        intent.intent.idempotencyKey = move.move_id;
        intent.intent.receiptSequence = 0; // Not used for testing
        intent.intent.hiderId = move.hider_id;
        intent.intent.mode = static_cast<HiderActionMode>(move.server_mode);
        
        if (applyTickIntent(tempState, intent) == RuleOutcome::ACCEPTED) {
            affordable = true;
            break;
        }
    }
    
    if (!affordable) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::NEVER_AFFORDABLE;
        return result;
    }
    
    // If we get here, the move is affordable but not immediately so
    // We'll queue it for later processing
    result.status = MoveStatus::QUEUED;
    result.reason = TurnReason::NONE;
    state.revision++;
    return result;
}

TurnResult queueMove(MatchState& state, const MoveRequest& move, iTargetRankingPolicy& seekerPolicy) {
    TurnResult result{};
    
    // Common checks
    if (state.phase != PHASE_ACTIVE) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::MATCH_NOT_ACTIVE;
        return result;
    }
    
    if (!isValidHider(state, move.hider_id)) {
        if (state.hiders[move.hider_id].status == HIDER_STATUS_CAPTURED) {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::HIDER_CAPTURED;
        } else {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::RULE_REJECTED;
            result.ruleOutcome = RuleOutcome::REJECTED_INVALID_TARGET;
        }
        return result;
    }
    
    // Check if queue is full
    if (state.pending_count >= MAX_PENDING_MOVES) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::QUEUE_FULL;
        return result;
    }
    
    // Check if hider already has too many pending moves
    if (hasTooManyPendingMoves(state, move.hider_id)) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::HIDER_QUEUE_LIMIT;
        return result;
    }
    
    // Check for duplicate move ID
    if (hasDuplicateMoveId(state, move.move_id)) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::DUPLICATE_MOVE_ID;
        return result;
    }
    
    // Add to pending moves
    int index = (state.pending_head + state.pending_count) % MAX_PENDING_MOVES;
    state.pending[index].move_id = move.move_id;
    state.pending[index].submit_seq = state.next_submit_seq;
    state.pending[index].hider_id = move.hider_id;
    state.pending[index].action_type = move.action_type;
    state.pending[index].server_id = move.server_id;
    state.pending[index].target_id = move.target_id;
    state.pending[index].server_mode = move.server_mode;
    
    state.pending_count++;
    state.next_submit_seq++;
    
    result.status = MoveStatus::QUEUED;
    result.reason = TurnReason::NONE;
    state.revision++;
    return result;
}

TurnResult passTicks(MatchState& state, uint8_t ticks, iTargetRankingPolicy& seekerPolicy) {
    TurnResult result{};
    
    // Common checks
    if (state.phase != PHASE_ACTIVE) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::MATCH_NOT_ACTIVE;
        return result;
    }
    
    if (ticks == 0 || ticks > MAX_ADVANCE_TICKS) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::INVALID_TICK_COUNT;
        return result;
    }
    
    // Advance the loop
    result = advanceLoop(state, nullptr, seekerPolicy, ticks);
    
    // If we're here, it means we advanced successfully
    if (result.ticksAdvanced > 0) {
        state.revision++;
    }
    
    return result;
}

TurnResult cancelPendingMove(MatchState& state, uint8_t hiderId, uint32_t moveId) {
    TurnResult result{};
    
    // Common checks
    if (state.phase != PHASE_ACTIVE) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::MATCH_NOT_ACTIVE;
        return result;
    }
    
    if (!isValidHider(state, hiderId)) {
        if (state.hiders[hiderId].status == HIDER_STATUS_CAPTURED) {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::HIDER_CAPTURED;
        } else {
            result.status = MoveStatus::REJECTED;
            result.reason = TurnReason::RULE_REJECTED;
            result.ruleOutcome = RuleOutcome::REJECTED_INVALID_TARGET;
        }
        return result;
    }
    
    // Find the pending move
    int index = findPendingMove(state, hiderId, moveId);
    if (index == -1) {
        result.status = MoveStatus::REJECTED;
        result.reason = TurnReason::NOT_FOUND;
        return result;
    }
    
    // Mark as cancelled and remove
    state.pending[index] = PendingMove{};
    state.pending_count--;
    
    result.status = MoveStatus::CANCELLED;
    result.reason = TurnReason::NONE;
    state.revision++;
    return result;
}

} // namespace hider_seeker