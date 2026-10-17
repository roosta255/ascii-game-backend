#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "state.hpp"
#include "serialization.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"
#include "probe_rules.hpp"
#include "tick_types.hpp"
#include "turn_types.hpp"
#include "turn_engine.hpp"

using namespace hider_seeker;

namespace {

// Seeker policy stub that only counts how often it is consulted.
struct CountingPolicy : public iTargetRankingPolicy {
    mutable int calls = 0;
    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>&) const override {
        calls++;
    }
};

MatchState makeActiveMatch(uint8_t hiderCount) {
    MatchState state{};
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = hiderCount;
    for (uint8_t hiderId = 0; hiderId < hiderCount; ++hiderId) {
        state.hiders[hiderId].active = 1;
        state.hiders[hiderId].status = HIDER_STATUS_ACTIVE;
        // 5a1b: hider actions need a connection; an inactive server keeps the old no-server behavior
        state.hiders[hiderId].server_id = 0; // Set to inactive server (server 0 is inactive in these tests)
    }
    return state;
}

MoveRequest hiderAction(uint32_t moveId, uint8_t hiderId, HiderActionMode mode) {
    return MoveRequest{moveId, hiderId, static_cast<uint8_t>(TickActionKind::HIDER_ACTION),
                       NO_SERVER_ID, NO_SERVER_ID, static_cast<uint8_t>(mode)};
}

std::vector<uint8_t> serialize(const MatchState& state) {
    std::vector<uint8_t> out;
    saveMatchState(state, out);
    return out;
}

PendingMoveApplied appliedAt(const TurnResult& result, int index) {
    PendingMoveApplied applied{0, NO_SERVER_ID, 0};
    result.pendingApplied.accessConst(index, [&applied](const PendingMoveApplied& entry) { applied = entry; });
    return applied;
}

// A fixed sequence of turn-layer calls used by the determinism test.
void runScriptedSequence(MatchState& state) {
    CountingPolicy policy;
    queueMove(state, hiderAction(1, 0, HiderActionMode::CONFIGURE_SERVER), policy);
    queueMove(state, hiderAction(2, 1, HiderActionMode::ROTATE_ROUTE), policy);
    queueMove(state, hiderAction(3, 1, HiderActionMode::PAY_GOAL), policy);
    playMove(state, hiderAction(4, 2, HiderActionMode::PAY_GOAL), policy);
    passTicks(state, 3, policy);
    cancelPendingMove(state, 1, 3);
    playMove(state, hiderAction(5, 0, HiderActionMode::BOOST_CPU), policy);
    passTicks(state, 16, policy);
}

} // namespace

TEST_CASE("TURN-007 passTicks(3) advances 3 ticks and applies affordable pending moves", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 30;
    CountingPolicy policy;

    REQUIRE(queueMove(state, hiderAction(1, 0, HiderActionMode::SUPPRESS_ERROR), policy).status == MoveStatus::QUEUED);
    const uint64_t revisionBefore = state.revision;

    const TurnResult result = passTicks(state, 3, policy);

    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(result.ticksAdvanced == 3);
    REQUIRE(state.tick == 33);
    REQUIRE(policy.calls == 3);
    REQUIRE(state.revision == revisionBefore + 1);

    // SUPPRESS_ERROR (cost 3) becomes affordable on the second tick (4 AP)
    REQUIRE(result.pendingAppliedCount == 1);
    REQUIRE(appliedAt(result, 0).move_id == 1);
    REQUIRE(appliedAt(result, 0).tick == 32);
    REQUIRE(state.pending_count == 0);
    REQUIRE(state.hiders[0].action_points == 3); // 4 - 3 + 2
}

TEST_CASE("TURN-007 passTicks rejects 0 and more than MAX_ADVANCE_TICKS", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 30;
    CountingPolicy policy;
    const std::vector<uint8_t> before = serialize(state);

    const TurnResult zero = passTicks(state, 0, policy);
    REQUIRE(zero.status == MoveStatus::REJECTED);
    REQUIRE(zero.reason == TurnReason::INVALID_TICK_COUNT);
    REQUIRE(serialize(state) == before);

    const TurnResult tooMany = passTicks(state, 17, policy);
    REQUIRE(tooMany.status == MoveStatus::REJECTED);
    REQUIRE(tooMany.reason == TurnReason::INVALID_TICK_COUNT);
    REQUIRE(serialize(state) == before);

    REQUIRE(policy.calls == 0);
}

TEST_CASE("TURN-007 identical call sequences produce byte-identical state", "[hider_seeker][turn]") {
    MatchState initial = makeActiveMatch(3);
    initial.tick = 5;
    initial.rng_state = 0x1234ABCDULL;
    initial.seeker.max_tokens = 4;
    initial.seeker.connection_tokens = 1;
    initial.servers[0].active = 1;
    initial.server_count = 1;
    initial.hiders[0].server_id = 0;

    MatchState first = initial;
    MatchState second = initial;
    runScriptedSequence(first);
    runScriptedSequence(second);

    REQUIRE(first.tick > initial.tick);
    REQUIRE(serialize(first) == serialize(second));
}
