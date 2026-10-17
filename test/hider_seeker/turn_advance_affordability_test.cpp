#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "state.hpp"
#include "serialization.hpp"
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

std::vector<uint8_t> serialize(const MatchState& state) {
    std::vector<uint8_t> out;
    saveMatchState(state, out);
    return out;
}

} // namespace

TEST_CASE("TURN-004 playMove PAY_GOAL with 0 AP applies on tick 5", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 40;
    state.hiders[0].action_points = 0;
    CountingPolicy policy;

    const MoveRequest move{1, 0, static_cast<uint8_t>(TickActionKind::HIDER_ACTION), NO_SERVER_ID,
                           NO_SERVER_ID, static_cast<uint8_t>(HiderActionMode::PAY_GOAL)};
    const TurnResult result = playMove(state, move, policy);

    REQUIRE(result.status == MoveStatus::APPLIED);
    REQUIRE(result.reason == TurnReason::NONE);
    REQUIRE(result.ruleOutcome == RuleOutcome::ACCEPTED);
    REQUIRE(result.ticksAdvanced == 5);
    REQUIRE(state.tick == 45);
    REQUIRE(state.hiders[0].action_points == 0);
    REQUIRE(policy.calls == 5);
    REQUIRE(state.revision == 1);
}

TEST_CASE("TURN-004 playMove ADD_SERVER with 0 LOA is NEVER_AFFORDABLE", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(1);
    state.tick = 40;
    state.revision = 3;
    state.next_submit_seq = 9;
    state.hiders[0].action_points = 0;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 0;
    CountingPolicy policy;

    const std::vector<uint8_t> before = serialize(state);

    const MoveRequest move{1, 0, static_cast<uint8_t>(TickActionKind::ADD_SERVER), NO_SERVER_ID,
                           NO_SERVER_ID, 0};
    const TurnResult result = playMove(state, move, policy);

    REQUIRE(result.status == MoveStatus::REJECTED);
    REQUIRE(result.reason == TurnReason::NEVER_AFFORDABLE);
    REQUIRE(result.ticksAdvanced == 0);
    REQUIRE(policy.calls == 0);
    REQUIRE(serialize(state) == before);
}
