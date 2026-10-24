#pragma once

#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include "../ruleset/rule_types.hpp"
#include "../ruleset/lifecycle_events.hpp"
#include "../tick/tick_types.hpp"

namespace hider_seeker {

// PLACEHOLDER pending BALANCE-001: the most moves a single hider may have
// waiting in MatchState::pending at once (governs TurnReason::HIDER_QUEUE_LIMIT).
constexpr uint8_t PLACEHOLDER_MAX_PENDING_PER_HIDER = 4;

// One move submitted by a hider.
struct MoveRequest {
    uint32_t move_id;
    uint8_t hider_id;
    uint8_t action_type; // TickActionKind
    uint8_t server_id;   // target server for REMOVE_SERVER, else NO_SERVER_ID
    uint8_t target_id;   // reserved, always NO_SERVER_ID for now
    uint8_t server_mode; // HiderActionMode, meaningful for HIDER_ACTION
};

enum class MoveStatus : uint8_t {
    APPLIED,
    QUEUED,
    REJECTED,
    CANCELLED
};

enum class TurnReason : uint8_t {
    NONE,
    NEVER_AFFORDABLE,
    QUEUE_FULL,
    HIDER_QUEUE_LIMIT,
    DUPLICATE_MOVE_ID,
    RULE_REJECTED,
    HIDER_CAPTURED,
    MATCH_NOT_ACTIVE,
    ADVANCE_LIMIT,
    NOT_FOUND,
    INVALID_TICK_COUNT,
    OWNER_CANCELLED
};

// A previously queued move that was applied during an advance.
struct PendingMoveApplied {
    uint32_t move_id;
    uint8_t hider_id;
    uint32_t tick; // MatchState::tick of the tick it was applied in
};

struct PendingMoveCancelled {
    uint32_t move_id;
    uint8_t hider_id;
    TurnReason reason;
    RuleOutcome ruleOutcome;
};

struct TurnResult {
    MoveStatus status = MoveStatus::REJECTED;
    TurnReason reason = TurnReason::NONE;
    uint8_t ticksAdvanced = 0;
    // Rule outcome of the submitted move (ACCEPTED when the call involves no
    // rule rejection).
    RuleOutcome ruleOutcome = RuleOutcome::ACCEPTED;
    // Public lifecycle events of every intent applied by this call, in order.
    Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> publicEvents;
    uint8_t publicEventCount = 0;
    Array<PendingMoveApplied, MAX_PENDING_MOVES> pendingApplied;
    uint8_t pendingAppliedCount = 0;
    Array<PendingMoveCancelled, MAX_PENDING_MOVES> pendingCancelled;
    uint8_t pendingCancelledCount = 0;
    uint32_t probesIssued = 0;
};

} // namespace hider_seeker