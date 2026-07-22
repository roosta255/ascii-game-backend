#pragma once

#include "BehaviorEnum.hpp"
#include "CodeEnum.hpp"

// Per-conduct FSM state slot. Variable data shared across all conducts lives
// in Conduct::vars — see Conduct.hpp.
struct ConductMemory {
    BehaviorEnum state         = BEHAVIOR_NIL;

    // Set whenever TriggerEffectSetBehavior changes state away from its prior value.
    BehaviorEnum previousState   = BEHAVIOR_NIL;
    int          stateChangedTurn = -1; // match.turner.turn at the time of that change

    // Priority of the most recent winning proposer (see Conduct::buildAndExecuteProposals).
    int latestPriority = -1;

    // codeset.error snapshotted right after this slot's effects / failureEffects last ran.
    // Reflects the shared per-activation Codeset, so a stale code from an unrelated
    // effect earlier in the same activation can still be showing here.
    CodeEnum lastEffectCode        = CODE_UNSET;
    CodeEnum lastFailureEffectCode = CODE_UNSET;
};
