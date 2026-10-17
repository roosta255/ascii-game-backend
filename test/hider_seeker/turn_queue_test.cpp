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

// move_id of the pending move `offset` places after pending_head.
uint32_t pendingIdAt(const MatchState& state, uint8_t offset) {
    return state.pending[(state.pending_head + offset) % MAX_PENDING_MOVES].move_id;
}

} // namespace

TEST_CASE("TURN-005 queued move applies mid-advance of another hider's playMove", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    state.tick = 10;
    CountingPolicy policy;

    // Hider A (0) queues CONFIGURE_SERVER (cost 5) with 0 AP
    const TurnResult queued = queueMove(state, hiderAction(1, 0, HiderActionMode::CONFIGURE_SERVER), policy);
    REQUIRE(queued.status == MoveStatus::QUEUED);
    REQUIRE(queued.reason == TurnReason::NONE);
    REQUIRE(queued.ticksAdvanced == 0);
    REQUIRE(state.tick == 10);
    REQUIRE(state.pending_count == 1);
    REQUIRE(state.next_submit_seq == 1);
    REQUIRE(policy.calls == 0);

    // Hider B (1) plays PAY_GOAL (cost 10) with 0 AP
    const TurnResult played = playMove(state, hiderAction(2, 1, HiderActionMode::PAY_GOAL), policy);
    REQUIRE(played.status == MoveStatus::APPLIED);
    REQUIRE(played.ticksAdvanced == 5);
    REQUIRE(state.tick == 15);

    // A's move applied on the third tick (A had 6 AP by then)
    REQUIRE(played.pendingAppliedCount == 1);
    const PendingMoveApplied applied = appliedAt(played, 0);
    REQUIRE(applied.move_id == 1);
    REQUIRE(applied.hider_id == 0);
    REQUIRE(applied.tick == 13);
    REQUIRE(state.pending_count == 0);

    // A: 6 - 5 = 1 at tick 3, then +2 on ticks 4 and 5. B: spent all 10.
    REQUIRE(state.hiders[0].action_points == 5);
    REQUIRE(state.hiders[1].action_points == 0);
}

TEST_CASE("TURN-005 same-hider competing queued moves apply in submit_seq order", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    CountingPolicy policy;

    // Submitted first despite the larger move_id
    REQUIRE(queueMove(state, hiderAction(20, 0, HiderActionMode::CONFIGURE_SERVER), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(10, 0, HiderActionMode::CONFIGURE_SERVER), policy).status == MoveStatus::QUEUED);

    const TurnResult result = passTicks(state, 5, policy);

    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.pendingAppliedCount == 2);
    REQUIRE(appliedAt(result, 0).move_id == 20);
    REQUIRE(appliedAt(result, 0).tick == 3);  // 6 AP -> 1
    REQUIRE(appliedAt(result, 1).move_id == 10);
    REQUIRE(appliedAt(result, 1).tick == 5);  // 1 + 2 + 2 = 5 AP -> 0
    REQUIRE(state.pending_count == 0);
}

TEST_CASE("TURN-005 17th queued move is rejected QUEUE_FULL", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(5);
    CountingPolicy policy;

    uint32_t moveId = 100;
    for (uint8_t hiderId = 0; hiderId < 4; ++hiderId) {
        for (uint8_t n = 0; n < PLACEHOLDER_MAX_PENDING_PER_HIDER; ++n) {
            REQUIRE(queueMove(state, hiderAction(moveId++, hiderId, HiderActionMode::PAY_GOAL), policy).status ==
                    MoveStatus::QUEUED);
        }
    }
    REQUIRE(state.pending_count == MAX_PENDING_MOVES);

    const std::vector<uint8_t> before = serialize(state);
    const TurnResult result = queueMove(state, hiderAction(moveId, 4, HiderActionMode::PAY_GOAL), policy);

    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::QUEUE_FULL);
    REQUIRE(serialize(state) == before);
}

TEST_CASE("TURN-005 5th queued move from one hider is rejected HIDER_QUEUE_LIMIT", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    CountingPolicy policy;

    for (uint32_t moveId = 1; moveId <= 4; ++moveId) {
        REQUIRE(queueMove(state, hiderAction(moveId, 0, HiderActionMode::PAY_GOAL), policy).status ==
                MoveStatus::QUEUED);
    }

    const std::vector<uint8_t> before = serialize(state);
    const TurnResult result = queueMove(state, hiderAction(5, 0, HiderActionMode::PAY_GOAL), policy);

    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::HIDER_QUEUE_LIMIT);
    REQUIRE(serialize(state) == before);
}

TEST_CASE("TURN-005 duplicate pending move_id is rejected DUPLICATE_MOVE_ID", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    CountingPolicy policy;

    REQUIRE(queueMove(state, hiderAction(7, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);

    const std::vector<uint8_t> before = serialize(state);
    const TurnResult result = queueMove(state, hiderAction(7, 1, HiderActionMode::PAY_GOAL), policy);

    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::DUPLICATE_MOVE_ID);
    REQUIRE(serialize(state) == before);
}

TEST_CASE("TURN-005 cancelPendingMove removes only the caller's own move", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    CountingPolicy policy;

    REQUIRE(queueMove(state, hiderAction(1, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(2, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(3, 0, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    REQUIRE(queueMove(state, hiderAction(4, 1, HiderActionMode::PAY_GOAL), policy).status == MoveStatus::QUEUED);
    const uint64_t revisionBefore = state.revision;

    const TurnResult cancelled = cancelPendingMove(state, 0, 2);
    REQUIRE(cancelled.status == MoveStatus::CANCELLED);
    REQUIRE(cancelled.reason == TurnReason::NONE);
    REQUIRE(state.revision == revisionBefore + 1);
    REQUIRE(state.pending_count == 3);
    REQUIRE(pendingIdAt(state, 0) == 1);
    REQUIRE(pendingIdAt(state, 1) == 3);
    REQUIRE(pendingIdAt(state, 2) == 4);

    // Move 1 belongs to hider 0, so hider 1 cannot cancel it
    const std::vector<uint8_t> before = serialize(state);
    const TurnResult otherHider = cancelPendingMove(state, 1, 1);
    REQUIRE(otherHider.status == MoveStatus::REJECTED);
    REQUIRE(otherHider.reason == TurnReason::NOT_FOUND);
    REQUIRE(serialize(state) == before);

    const TurnResult unknown = cancelPendingMove(state, 0, 99);
    REQUIRE(unknown.status == MoveStatus::REJECTED);
    REQUIRE(unknown.reason == TurnReason::NOT_FOUND);
    REQUIRE(serialize(state) == before);
}
