#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "rule_types.hpp"
#include "action_rules.hpp"
#include "hider_status.hpp"
#include "tick_types.hpp"
#include "turn_types.hpp"
#include "turn_engine.hpp"
#include "policy_interface.hpp"

using namespace hider_seeker;

namespace {

// Seeker policy stub that only counts how often it is consulted.
struct CountingPolicy : public iSeekerPolicy {
    mutable int calls = 0;
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override {
        calls++;
    }
    std::string policyId() const override { return "test"; }
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

} // namespace

TEST_CASE("TURN-003 affordable playMove applies immediately without advancing", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 100;
    state.revision = 7;
    state.hiders[0].action_points = 5;
    CountingPolicy policy;

    const TurnResult result = playMove(state, hiderAction(1, 0, HiderActionMode::BOOST_CPU), policy);

    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(result.ruleOutcome == RuleOutcome::ACCEPTED);
    REQUIRE(result.ticksAdvanced == 0);
    REQUIRE(policy.calls == 0);
    REQUIRE(state.tick == 100);
    REQUIRE(state.revision == 8);
    REQUIRE(state.hiders[0].action_points == 3); // BOOST_CPU costs 2
    REQUIRE(state.pending_count == 0);
}

TEST_CASE("TURN-003 affordable queueMove applies immediately without queueing", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 100;
    state.revision = 7;
    state.hiders[0].action_points = 5;
    CountingPolicy policy;

    const TurnResult result = queueMove(state, hiderAction(1, 0, HiderActionMode::SUPPRESS_ERROR), policy);

    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(result.ticksAdvanced == 0);
    REQUIRE(policy.calls == 0);
    REQUIRE(state.tick == 100);
    REQUIRE(state.revision == 8);
    REQUIRE(state.hiders[0].action_points == 2); // SUPPRESS_ERROR costs 3
    REQUIRE(state.pending_count == 0);
    REQUIRE(state.next_submit_seq == 0);
}
