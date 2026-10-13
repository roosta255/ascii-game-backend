#pragma once

#include "../hider_seeker/domain/state.hpp"
#include "../hider_seeker/tick/tick_types.hpp"
#include "../hider_seeker/ruleset/rule_types.hpp"

namespace hider_seeker {

struct MoveRequest {
    uint32_t move_id;
    uint8_t hider_id;
    uint8_t action_type; // TickActionKind
    uint8_t server_id;
    uint8_t target_id;
    uint8_t server_mode; // HiderActionMode
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
    INVALID_TICK_COUNT
};

struct PendingMoveApplied {
    uint32_t move_id;
    uint8_t hider_id;
    uint32_t tick;
};

struct TurnResult {
    MoveStatus status;
    TurnReason reason;
    uint8_t ticksAdvanced;
    RuleOutcome ruleOutcome;
    Array<PublicLifecycleEvent, MAX_EVENTS> publicEvents;
    uint8_t publicEventCount;
    Array<PendingMoveApplied, MAX_PENDING_MOVES> pendingApplied;
    uint8_t pendingAppliedCount;
    uint32_t probesIssued;
};

// Placeholder pending BALANCE-001
constexpr uint8_t PLACEHOLDER_MAX_PENDING_PER_HIDER = 4;

} // namespace hider_seeker