#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"
#include "target_ranking_interface.hpp"
#include "hider_status.hpp"

namespace hider_seeker {

// Ruleset-defined HiderState::status values used by probe resolution.

// ---------------------------------------------------------------------------
// Replay contract (RULESET-001) exposed by this ruleset slice for the future
// tick engine. This slice does not persist anything itself; it guarantees:
//  1. Every function here is a pure, deterministic function of its inputs
//     (MatchState + arguments). The only randomness is the seeded RNG whose
//     state lives in MatchState::rng_state, which applyProbe advances.
//  2. applyProbe fills ProbeResult with the complete record of its draw:
//     rngStateBefore (MatchState::rng_state before the draw), rngCounter (the
//     counter the draw used, = MatchState::tick) and randomValue, plus the
//     resulting hit decision. replayProbeDraw() recomputes randomValue/hit
//     from that record alone, so a persisted ProbeResult is self-verifying.
//  3. resolveProbeConsequences returns ProbeConsequences, the full record of
//     every capture/escape/goal/reward decision it applied.
// To make a committed tick replayable, the tick engine must persist, with the
// committed revision: the pre-tick MatchState (including rng_state), the
// accepted intents in orderAcceptedIntents order with their idempotency keys
// (the consumed-key set fed to isIntentAlreadyApplied), and for each probe its
// ProbeResult and ProbeConsequences.
// ---------------------------------------------------------------------------

struct ProbeResult {
    bool hit;
    uint8_t targetServerId;
    // Replay record of the RNG draw that decided `hit` (filled by applyProbe).
    uint64_t rngStateBefore;
    uint64_t rngCounter;
    uint32_t randomValue;
};

// Record of every decision resolveProbeConsequences applied.
// Bit h of a mask refers to hider slot h (MAX_HIDERS == 8 fits in a uint8_t).
struct ProbeConsequences {
    uint8_t capturedHiderMask;  // hiders captured by this probe
    uint8_t escapedHiderMask;   // hiders that escaped this probe
    uint8_t disqualifiedGoalProgressCount; // goal_progress entries reset by captures
    uint8_t tokensAwarded;      // connection tokens awarded to the seeker
};

RuleOutcome applyProbe(MatchState&, uint8_t serverId, ProbeResult& outResult);

// Pure: hit decision for a draw against a server's trace strength.
bool computeProbeHit(uint32_t randomValue, uint8_t traceStrength);

// Pure: recomputes the draw recorded in `record` (from rngStateBefore and
// rngCounter) and the hit decision against traceStrength. Returns true iff the
// recomputed randomValue and hit both match the record.
bool replayProbeDraw(const ProbeResult& record, uint8_t traceStrength);

// PROBE-001 consequence rule (placeholder numeric values, see probe_rules.cpp):
//  - Miss: seeker belief for the server is set to PLACEHOLDER_BELIEF_ON_MISS and
//    last_probe_tick is updated. Nothing else changes.
//  - Hit: belief set to PLACEHOLDER_BELIEF_ON_HIT and last_probe_tick updated.
//    Then each active, not-yet-captured hider occupying the server (server_id),
//    in ascending hider index order, is resolved:
//      * ESCAPE if it has >= PLACEHOLDER_ESCAPE_AP_COST action points AND an
//        escape server exists (lowest-numbered active server other than the
//        probed one): it pays the AP and moves to that server.
//      * otherwise CAPTURE: status = HIDER_STATUS_CAPTURED; goal qualification
//        is forfeited (all of its non-zero goal_progress entries reset to 0);
//        the seeker is rewarded PLACEHOLDER_CAPTURE_TOKEN_REWARD connection
//        tokens, capped at max_tokens.
//  - An out-of-range targetServerId has no effect.
ProbeConsequences resolveProbeConsequences(MatchState&, const ProbeResult&);

} // namespace hider_seeker