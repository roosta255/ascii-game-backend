#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"
#include "loa_rules.hpp"
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

// Seeker policy stub that captures one hider of the match it is bound to on
// its Nth consultation (i.e. during the Nth advanced tick).
struct CapturingPolicy : public iTargetRankingPolicy {
    CapturingPolicy(MatchState& match, uint8_t hiderId, int captureOnCall)
    : match(match), hiderId(hiderId), captureOnCall(captureOnCall) {}

    void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>&) const override {
        calls++;
        if (calls == captureOnCall) {
            match.hiders[hiderId].status = HIDER_STATUS_CAPTURED;
        }
    }

    MatchState& match;
    uint8_t hiderId;
    int captureOnCall;
    mutable int calls = 0;
};

MatchState makeActiveMatch(uint8_t hiderCount) {
    MatchState state{};
    state.schema_version = 2;
    state.phase = PHASE_ACTIVE;
    state.hider_count = hiderCount;
    for (uint8_t hiderId = 0; hiderId < hiderCount; ++hiderId) {
        state.hiders[hiderId].active = 1;
        state.hiders[hiderId].status = HIDER_STATUS_ACTIVE;
        state.hiders[hiderId].server_id = NO_SERVER_ID;
    }
    return state;
}

MoveRequest hiderAction(uint32_t moveId, uint8_t hiderId, HiderActionMode mode) {
    return MoveRequest{moveId, hiderId, static_cast<uint8_t>(TickActionKind::HIDER_ACTION),
                       NO_SERVER_ID, NO_SERVER_ID, static_cast<uint8_t>(mode)};
}

MoveRequest removeServer(uint32_t moveId, uint8_t hiderId, uint8_t serverId) {
    return MoveRequest{moveId, hiderId, static_cast<uint8_t>(TickActionKind::REMOVE_SERVER),
                       serverId, NO_SERVER_ID, 0};
}

PendingMoveApplied appliedAt(const TurnResult& result, int index) {
    PendingMoveApplied applied{0, NO_SERVER_ID, 0};
    result.pendingApplied.accessConst(index, [&applied](const PendingMoveApplied& entry) { applied = entry; });
    return applied;
}

uint32_t pendingIdAt(const MatchState& state, uint8_t offset) {
    return state.pending[(state.pending_head + offset) % MAX_PENDING_MOVES].move_id;
}

} // namespace

TEST_CASE("TURN-006 queued REMOVE_SERVER whose target is removed is cancelled RULE_REJECTED", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    state.servers[0].active = 1;
    state.servers[3].active = 1;
    state.server_count = 2;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 0;
    state.hiders[1].resources[LOA_RESOURCE_INDEX] = 1;
    CountingPolicy policy;

    // Hider 0 cannot afford the removal yet (0 LOA), so it waits
    const TurnResult queued = queueMove(state, removeServer(1, 0, 3), policy);
    REQUIRE(queued.status == MoveStatus::QUEUED);
    REQUIRE(state.pending_count == 1);

    // Hider 1 removes the same slot first
    const TurnResult removed = queueMove(state, removeServer(2, 1, 3), policy);
    REQUIRE(removed.status == MoveStatus::APPLIED);
    REQUIRE(state.servers[3].active == 0);
    REQUIRE(state.pending_count == 1);

    // On the next tick hider 0's move is rule-rejected and cancelled
    const TurnResult passed = passTicks(state, 1, policy);
    REQUIRE(passed.status == MoveStatus::APPLIED);
    REQUIRE(passed.ticksAdvanced == 1);
    REQUIRE(passed.pendingAppliedCount == 0);
    REQUIRE(state.pending_count == 0);
    REQUIRE(state.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);
}

TEST_CASE("TURN-006 hider captured mid-advance cancels its moves HIDER_CAPTURED", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    state.tick = 20;
    CapturingPolicy policy(state, 0, 2);

    REQUIRE(queueMove(state, hiderAction(1, 0, HiderActionMode::CONFIGURE_SERVER), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(2, 1, HiderActionMode::CONFIGURE_SERVER), policy).status == MoveStatus::QUEUED);
    const uint64_t revisionBefore = state.revision;

    const TurnResult result = playMove(state, hiderAction(3, 0, HiderActionMode::PAY_GOAL), policy);

    REQUIRE(result.status == MoveStatus::CANCELLED);
    REQUIRE(result.reason == TurnReason::HIDER_CAPTURED);
    REQUIRE(result.ticksAdvanced == 2);
    REQUIRE(policy.calls == 2);

    // The two ticks that ran stay applied
    REQUIRE(state.tick == 22);
    REQUIRE(state.hiders[0].action_points == 4);
    REQUIRE(state.hiders[1].action_points == 4);
    REQUIRE(state.revision == revisionBefore + 1);

    // Hider 0's pending move is gone; hider 1's is still waiting
    REQUIRE(state.pending_count == 1);
    REQUIRE(pendingIdAt(state, 0) == 2);
}

TEST_CASE("TURN-006 advance stops at MAX_ADVANCE_TICKS with ADVANCE_LIMIT", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    CountingPolicy policy;

    REQUIRE(queueMove(state, hiderAction(1, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(2, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(3, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);

    const TurnResult result = playMove(state, hiderAction(4, 0, HiderActionMode::PAY_GOAL), policy);

    REQUIRE(result.status == MoveStatus::CANCELLED);
    REQUIRE(result.reason == TurnReason::ADVANCE_LIMIT);
    REQUIRE(result.ticksAdvanced == MAX_ADVANCE_TICKS);
    REQUIRE(result.ticksAdvanced == 16);
    REQUIRE(state.tick == 16);
    REQUIRE(policy.calls == 16);

    REQUIRE(result.pendingAppliedCount == 3);
    REQUIRE(appliedAt(result, 0).move_id == 1);
    REQUIRE(appliedAt(result, 0).tick == 5);
    REQUIRE(appliedAt(result, 1).move_id == 2);
    REQUIRE(appliedAt(result, 1).tick == 10);
    REQUIRE(appliedAt(result, 2).move_id == 3);
    REQUIRE(appliedAt(result, 2).tick == 15);

    REQUIRE(state.pending_count == 0);
    REQUIRE(state.hiders[0].action_points == 2);
}
