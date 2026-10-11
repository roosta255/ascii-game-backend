#include <catch2/catch_test_macros.hpp>
#include "probe_rules.hpp"
#include "state.hpp"
#include "target_ranking_interface.hpp"

using namespace hider_seeker;

TEST_CASE("applyProbe deducts token cost before computing hit/miss result", "[probe]") {
    MatchState match = {};
    
    // Initialize seeker with 2 tokens
    match.seeker.connection_tokens = 2;
    
    ProbeResult result;
    RuleOutcome outcome = applyProbe(match, 0, result);
    
    // Should succeed and deduct 1 token
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(match.seeker.connection_tokens == 1);
}

TEST_CASE("applyProbe is rejected when insufficient tokens", "[probe]") {
    MatchState match = {};
    
    // Initialize seeker with 0 tokens
    match.seeker.connection_tokens = 0;
    
    ProbeResult result;
    RuleOutcome outcome = applyProbe(match, 0, result);
    
    // Should fail due to insufficient tokens
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(match.seeker.connection_tokens == 0);
}

namespace {

// Two active servers (0 = probed, 2 = escape destination); hiders 0 and 1
// occupy server 0, hider 2 occupies server 2.
MatchState makeProbeConsequenceMatch() {
    MatchState match = {};
    match.tick = 37;
    match.servers[0].active = 1;
    match.servers[2].active = 1;
    match.seeker.connection_tokens = 3;
    match.seeker.max_tokens = 10;
    for (uint8_t h = 0; h < 3; ++h) {
        match.hiders[h].active = 1;
        match.hiders[h].status = HIDER_STATUS_ACTIVE;
    }
    match.hiders[0].server_id = 0;
    match.hiders[1].server_id = 0;
    match.hiders[2].server_id = 2;
    return match;
}

ProbeResult makeResult(bool hit, uint8_t serverId) {
    ProbeResult result = {};
    result.hit = hit;
    result.targetServerId = serverId;
    return result;
}

} // namespace

TEST_CASE("resolveProbeConsequences on a hit captures or lets occupants escape", "[probe]") {
    MatchState match = makeProbeConsequenceMatch();
    match.hiders[0].action_points = 0;   // cannot pay escape cost -> captured
    match.hiders[0].goal_progress[1] = 5;
    match.hiders[0].goal_progress[4] = 2;
    match.hiders[1].action_points = 5;   // can pay escape cost (3) -> escapes
    match.hiders[1].goal_progress[1] = 6;
    match.hiders[2].action_points = 0;   // not on the probed server -> untouched

    ProbeConsequences c = resolveProbeConsequences(match, makeResult(true, 0));

    // Hider 0 captured, goal progress forfeited.
    REQUIRE(c.capturedHiderMask == 0x01);
    REQUIRE(match.hiders[0].status == HIDER_STATUS_CAPTURED);
    REQUIRE(match.hiders[0].server_id == 0);
    REQUIRE(match.hiders[0].goal_progress[1] == 0);
    REQUIRE(match.hiders[0].goal_progress[4] == 0);
    REQUIRE(c.disqualifiedGoalProgressCount == 2);

    // Hider 1 escaped to the lowest-numbered other active server, paying AP.
    REQUIRE(c.escapedHiderMask == 0x02);
    REQUIRE(match.hiders[1].status == HIDER_STATUS_ACTIVE);
    REQUIRE(match.hiders[1].server_id == 2);
    REQUIRE(match.hiders[1].action_points == 2);
    REQUIRE(match.hiders[1].goal_progress[1] == 6);

    // Hider 2 untouched.
    REQUIRE(match.hiders[2].status == HIDER_STATUS_ACTIVE);
    REQUIRE(match.hiders[2].server_id == 2);

    // Seeker reward for one capture, and updated knowledge.
    REQUIRE(c.tokensAwarded == 2);
    REQUIRE(match.seeker.connection_tokens == 5);
    REQUIRE(match.seeker.belief[0] == 200);
    REQUIRE(match.seeker.last_probe_tick[0] == 37);
}

TEST_CASE("resolveProbeConsequences captures when no escape server exists", "[probe]") {
    MatchState match = makeProbeConsequenceMatch();
    match.servers[2].active = 0; // no other active server
    match.hiders[1].action_points = 5;
    match.seeker.connection_tokens = 9; // reward capped at max_tokens (10)

    ProbeConsequences c = resolveProbeConsequences(match, makeResult(true, 0));

    REQUIRE(c.capturedHiderMask == 0x03);
    REQUIRE(c.escapedHiderMask == 0);
    REQUIRE(match.hiders[1].status == HIDER_STATUS_CAPTURED);
    REQUIRE(match.hiders[1].action_points == 5);
    REQUIRE(match.seeker.connection_tokens == 10);
    REQUIRE(c.tokensAwarded == 1);
}

TEST_CASE("resolveProbeConsequences does not re-capture an already captured hider", "[probe]") {
    MatchState match = makeProbeConsequenceMatch();
    match.hiders[0].status = HIDER_STATUS_CAPTURED;
    match.hiders[1].status = HIDER_STATUS_CAPTURED;

    ProbeConsequences c = resolveProbeConsequences(match, makeResult(true, 0));

    REQUIRE(c.capturedHiderMask == 0);
    REQUIRE(c.tokensAwarded == 0);
    REQUIRE(match.seeker.connection_tokens == 3);
}

TEST_CASE("resolveProbeConsequences on a miss only updates seeker knowledge", "[probe]") {
    MatchState match = makeProbeConsequenceMatch();
    match.seeker.belief[0] = 120;

    ProbeConsequences c = resolveProbeConsequences(match, makeResult(false, 0));

    REQUIRE(c.capturedHiderMask == 0);
    REQUIRE(c.escapedHiderMask == 0);
    REQUIRE(c.tokensAwarded == 0);
    REQUIRE(match.seeker.belief[0] == 0);
    REQUIRE(match.seeker.last_probe_tick[0] == 37);
    REQUIRE(match.seeker.connection_tokens == 3);
    REQUIRE(match.hiders[0].status == HIDER_STATUS_ACTIVE);
    REQUIRE(match.hiders[1].status == HIDER_STATUS_ACTIVE);
    REQUIRE(match.hiders[0].server_id == 0);
}

TEST_CASE("applyProbe records a replayable draw (RULESET-001)", "[probe]") {
    MatchState match = {};
    match.tick = 12;
    match.rng_state = 0x1234ABCDULL;
    match.seeker.connection_tokens = 4;
    match.servers[3].active = 1;
    match.servers[3].trace_strength = 50;
    MatchState copy = match;

    ProbeResult result = {};
    REQUIRE(applyProbe(match, 3, result) == RuleOutcome::ACCEPTED);
    REQUIRE(result.targetServerId == 3);
    REQUIRE(result.rngStateBefore == 0x1234ABCDULL);
    REQUIRE(result.rngCounter == 12);
    REQUIRE(result.hit == computeProbeHit(result.randomValue, 50));
    REQUIRE(match.rng_state != 0x1234ABCDULL); // RNG stream advanced

    // The record alone reproduces the draw and decision.
    REQUIRE(replayProbeDraw(result, 50));
    ProbeResult tampered = result;
    tampered.randomValue ^= 1u;
    REQUIRE_FALSE(replayProbeDraw(tampered, 50));

    // Re-running from the same pre-state yields an identical result and RNG state.
    ProbeResult again = {};
    REQUIRE(applyProbe(copy, 3, again) == RuleOutcome::ACCEPTED);
    REQUIRE(again.hit == result.hit);
    REQUIRE(again.randomValue == result.randomValue);
    REQUIRE(copy.rng_state == match.rng_state);
}

namespace {

// Stub ranking policy (TOKEN-001). Its only output channel is the
// Array<ProbeCandidate, MAX_SERVERS> it fills (serverId + score): it has no
// field through which it could express a token cost or a hit/miss decision.
// It deliberately ranks servers adversarially (descending server id, huge
// scores) and counts its invocations.
struct StubTargetRankingPolicy : public iTargetRankingPolicy {
    mutable int calls = 0;
    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const override {
        ++calls;
        for (int i = 0; i < outCandidates.size(); ++i) {
            ProbeCandidate candidate = {};
            candidate.serverId = static_cast<uint8_t>(MAX_SERVERS - 1 - i);
            candidate.score = 1000000 - i;
            outCandidates.assignValue(i, candidate);
        }
    }
};

MatchState makePolicySeparationMatch() {
    MatchState match = makeProbeConsequenceMatch();
    match.rng_state = 0xC0FFEE1234ULL;
    match.servers[0].trace_strength = 60;
    match.hiders[0].action_points = 0;
    match.hiders[1].action_points = 5;
    return match;
}

} // namespace

TEST_CASE("iTargetRankingPolicy cannot influence probe token cost, hit/miss or consequences (TOKEN-001)", "[probe]") {
    StubTargetRankingPolicy stubPolicy;
    const iTargetRankingPolicy& policy = stubPolicy;

    // Baseline: no policy involved at all.
    MatchState baseline = makePolicySeparationMatch();
    ProbeResult baselineResult = {};
    RuleOutcome baselineOutcome = applyProbe(baseline, 0, baselineResult);
    ProbeConsequences baselineConsequences = resolveProbeConsequences(baseline, baselineResult);

    // Same pre-state, but the stub policy is consulted first and ranks a
    // different server on top. It only sees a const MatchState and only
    // writes ProbeCandidate{serverId, score}.
    MatchState withPolicy = makePolicySeparationMatch();
    Array<ProbeCandidate, MAX_SERVERS> candidates;
    policy.rankTargets(withPolicy, candidates);
    REQUIRE(stubPolicy.calls == 1);
    const ProbeCandidate fallback = {};
    const Array<ProbeCandidate, MAX_SERVERS>& constCandidates = candidates;
    REQUIRE(constCandidates.getOrDefault(0, fallback).serverId == MAX_SERVERS - 1);

    ProbeResult policyResult = {};
    RuleOutcome policyOutcome = applyProbe(withPolicy, 0, policyResult);
    ProbeConsequences policyConsequences = resolveProbeConsequences(withPolicy, policyResult);

    // applyProbe/resolveProbeConsequences take no policy and never invoke it.
    REQUIRE(stubPolicy.calls == 1);

    // Token cost is identical (fixed deduction before the draw).
    REQUIRE(baselineOutcome == RuleOutcome::ACCEPTED);
    REQUIRE(policyOutcome == baselineOutcome);
    REQUIRE(withPolicy.seeker.connection_tokens == baseline.seeker.connection_tokens);

    // Hit/miss result and its RNG record are identical.
    REQUIRE(policyResult.hit == baselineResult.hit);
    REQUIRE(policyResult.targetServerId == baselineResult.targetServerId);
    REQUIRE(policyResult.rngStateBefore == baselineResult.rngStateBefore);
    REQUIRE(policyResult.rngCounter == baselineResult.rngCounter);
    REQUIRE(policyResult.randomValue == baselineResult.randomValue);
    REQUIRE(policyResult.hit == computeProbeHit(policyResult.randomValue, 60));
    REQUIRE(withPolicy.rng_state == baseline.rng_state);

    // Consequences are identical.
    REQUIRE(policyConsequences.capturedHiderMask == baselineConsequences.capturedHiderMask);
    REQUIRE(policyConsequences.escapedHiderMask == baselineConsequences.escapedHiderMask);
    REQUIRE(policyConsequences.disqualifiedGoalProgressCount == baselineConsequences.disqualifiedGoalProgressCount);
    REQUIRE(policyConsequences.tokensAwarded == baselineConsequences.tokensAwarded);
    for (uint8_t h = 0; h < MAX_HIDERS; ++h) {
        REQUIRE(withPolicy.hiders[h].status == baseline.hiders[h].status);
        REQUIRE(withPolicy.hiders[h].server_id == baseline.hiders[h].server_id);
        REQUIRE(withPolicy.hiders[h].action_points == baseline.hiders[h].action_points);
    }
    REQUIRE(withPolicy.seeker.belief[0] == baseline.seeker.belief[0]);
    REQUIRE(withPolicy.seeker.last_probe_tick[0] == baseline.seeker.last_probe_tick[0]);
}