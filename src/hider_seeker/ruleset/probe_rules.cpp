#include "probe_rules.hpp"
#include "rng.hpp"
#include <cstdint>

namespace hider_seeker {

// Placeholder constants for probe behavior - TODO: replace with final values
// These are placeholders as the design doc only gives hedged examples
constexpr uint8_t PLACEHOLDER_TOKEN_COST_PER_PROBE = 1;
// Probe consequence placeholders (PROBE-001) - TODO: replace with final values.
constexpr uint8_t PLACEHOLDER_BELIEF_ON_HIT = 200;
constexpr uint8_t PLACEHOLDER_BELIEF_ON_MISS = 0;
constexpr uint8_t PLACEHOLDER_ESCAPE_AP_COST = 3;
constexpr uint8_t PLACEHOLDER_CAPTURE_TOKEN_REWARD = 2;
// Hit chance is (traceStrength / PROBE_HIT_ROLL_RANGE), capped at 100%.
constexpr uint32_t PROBE_HIT_ROLL_RANGE = 100;

bool computeProbeHit(uint32_t randomValue, uint8_t traceStrength) {
    return (randomValue % PROBE_HIT_ROLL_RANGE) < traceStrength;
}

bool replayProbeDraw(const ProbeResult& record, uint8_t traceStrength) {
    RngState rng;
    rng.state = record.rngStateBefore;
    rng.counter = record.rngCounter;
    const uint32_t randomValue = nextRandomValue(rng);
    return randomValue == record.randomValue
        && computeProbeHit(randomValue, traceStrength) == record.hit;
}

RuleOutcome applyProbe(MatchState& match, uint8_t serverId, ProbeResult& outResult) {
    if (serverId >= MAX_SERVERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Deduct the backend-defined per-probe connection-token cost first
    if (match.seeker.connection_tokens < PLACEHOLDER_TOKEN_COST_PER_PROBE) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    match.seeker.connection_tokens -= PLACEHOLDER_TOKEN_COST_PER_PROBE;

    // Compute hit/miss result as a deterministic function of MatchState plus the
    // server-held seeded RNG (rng.hpp). The draw consumes the match's RNG stream
    // (match.rng_state is advanced), so the sequence of probe results is fully
    // reproducible from the initial seed and the ordered action log.
    RngState rng;
    rng.state = match.rng_state;
    rng.counter = match.tick;
    outResult.rngStateBefore = rng.state;
    outResult.rngCounter = rng.counter;
    const uint32_t randomValue = nextRandomValue(rng);
    match.rng_state = rng.state;

    // Higher trace strength = higher chance of hit.
    outResult.randomValue = randomValue;
    outResult.targetServerId = serverId;
    outResult.hit = computeProbeHit(randomValue, match.servers[serverId].trace_strength);

    return RuleOutcome::ACCEPTED;
}

ProbeConsequences resolveProbeConsequences(MatchState& match, const ProbeResult& result) {
    // PROBE-001: deterministic capture/escape/goal-qualification/reward rule,
    // documented in probe_rules.hpp.
    ProbeConsequences consequences = {0, 0, 0, 0};
    const uint8_t target = result.targetServerId;
    if (target >= MAX_SERVERS) {
        return consequences;
    }

    // SeekerState::last_probe_tick is a uint8_t field in the frozen domain layer,
    // so it stores the low byte of the tick.
    match.seeker.last_probe_tick[target] = static_cast<uint8_t>(match.tick);

    if (!result.hit) {
        match.seeker.belief[target] = PLACEHOLDER_BELIEF_ON_MISS;
        return consequences;
    }

    match.seeker.belief[target] = PLACEHOLDER_BELIEF_ON_HIT;

    // Escape destination: lowest-numbered active server other than the target.
    uint8_t escapeServerId = NO_SERVER_ID;
    for (uint8_t s = 0; s < MAX_SERVERS; ++s) {
        if (s != target && match.servers[s].active) {
            escapeServerId = s;
            break;
        }
    }

    for (uint8_t h = 0; h < MAX_HIDERS; ++h) {
        HiderState& hider = match.hiders[h];
        if (!hider.active || hider.status == HIDER_STATUS_CAPTURED || hider.server_id != target) {
            continue;
        }

        const uint8_t bit = static_cast<uint8_t>(1u << h);
        if (escapeServerId != NO_SERVER_ID && hider.action_points >= PLACEHOLDER_ESCAPE_AP_COST) {
            // Escape: pay AP and move to the escape server.
            hider.action_points -= PLACEHOLDER_ESCAPE_AP_COST;
            hider.server_id = escapeServerId;
            consequences.escapedHiderMask |= bit;
            continue;
        }

        // Capture.
        hider.status = HIDER_STATUS_CAPTURED;
        consequences.capturedHiderMask |= bit;

        // Goal qualification: a captured hider forfeits all goal progress.
        for (uint8_t g = 0; g < MAX_GOALS; ++g) {
            if (hider.goal_progress[g] != 0) {
                hider.goal_progress[g] = 0;
                consequences.disqualifiedGoalProgressCount++;
            }
        }

        // Reward: seeker gains connection tokens, capped at max_tokens.
        const uint8_t tokens = match.seeker.connection_tokens;
        const uint8_t cap = match.seeker.max_tokens;
        const uint8_t room = tokens < cap ? static_cast<uint8_t>(cap - tokens) : 0;
        const uint8_t award = PLACEHOLDER_CAPTURE_TOKEN_REWARD < room ? PLACEHOLDER_CAPTURE_TOKEN_REWARD : room;
        match.seeker.connection_tokens = static_cast<uint8_t>(tokens + award);
        consequences.tokensAwarded = static_cast<uint8_t>(consequences.tokensAwarded + award);
    }

    return consequences;
}

} // namespace hider_seeker