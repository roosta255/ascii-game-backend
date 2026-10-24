#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"
#include "loa_rules.hpp"
#include "hider_status.hpp"
#include "tick_types.hpp"
#include "turn_types.hpp"
#include "turn_engine.hpp"
#include "policy_interface.hpp"

using namespace hider_seeker;

namespace {

// Seeker policy stub that captures one hider of the match it is bound to on
// its Nth consultation (i.e. during the Nth advanced tick).
struct CapturingPolicy : public iSeekerPolicy {
    CapturingPolicy(MatchState& match, uint8_t hiderId, int captureOnCall)
    : match(match), hiderId(hiderId), captureOnCall(captureOnCall) {}

    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override {
        calls++;
        if (calls == captureOnCall) {
            match.hiders[hiderId].status = HIDER_STATUS_CAPTURED;
        }
    }
    std::string policyId() const override { return "test"; }

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
        // 5a1b: hider actions need a connection; an inactive server keeps the old no-server behavior
        state.hiders[hiderId].server_id = 0; // Set to inactive server (server 0 is inactive in these tests)
    }
    state.seeker.connection_tokens = 12; // 5a5b: a seeker with no pool and no connection has lost (checkMatchEnd)
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

} // namespace

TEST_CASE("TURN-012a: verify engine cancellation recording", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    state.tick = 20;
    state.servers[0].active = 1;
    state.servers[1].active = 1;
    state.server_count = 2;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 0;
    state.hiders[1].resources[LOA_RESOURCE_INDEX] = 0;
    CapturingPolicy policy(state, 0, 2);

    // Hider 0 queues CONFIGURE_SERVER (5 AP) with 0 AP: stored pending, no tick.
    const TurnResult queued0 = queueMove(state, hiderAction(1, 0, HiderActionMode::CONFIGURE_SERVER), policy);
    REQUIRE(queued0.status == MoveStatus::QUEUED);
    REQUIRE(state.pending_count == 1);

    // Hider 1 queues CONFIGURE_SERVER (5 AP) with 0 AP: stored pending, no tick.
    const TurnResult queued1 = queueMove(state, hiderAction(2, 1, HiderActionMode::CONFIGURE_SERVER), policy);
    REQUIRE(queued1.status == MoveStatus::QUEUED);
    REQUIRE(state.pending_count == 2);

    // Hider 0 plays PAY_GOAL (10 AP) with 0 AP: becomes affordable after 3 ticks.
    const TurnResult played = playMove(state, hiderAction(3, 0, HiderActionMode::PAY_GOAL), policy);
    REQUIRE(played.status == MoveStatus::CANCELLED);
    // Hider 0 is captured on the second tick: its PLAY move and its pending move 1 are cancelled HIDER_CAPTURED
    REQUIRE(state.pending_count == 1); // Only hider 1's move remains
    REQUIRE(played.pendingCancelledCount == 1);
    REQUIRE(played.pendingCancelled.getOrDefault(0, PendingMoveCancelled{}).move_id == 1);
    REQUIRE(played.pendingCancelled.getOrDefault(0, PendingMoveCancelled{}).hider_id == 0);
    REQUIRE(played.pendingCancelled.getOrDefault(0, PendingMoveCancelled{}).reason == TurnReason::HIDER_CAPTURED);

    // Cancel hider 1's move
    const TurnResult cancelled = cancelPendingMove(state, 1, 2);
    REQUIRE(cancelled.reason == TurnReason::NONE);
    REQUIRE(state.pending_count == 0); // No moves remain
    REQUIRE(cancelled.pendingCancelledCount == 1);
    REQUIRE(cancelled.pendingCancelled.getOrDefault(0, PendingMoveCancelled{}).move_id == 2);
    REQUIRE(cancelled.pendingCancelled.getOrDefault(0, PendingMoveCancelled{}).reason == TurnReason::OWNER_CANCELLED);

}