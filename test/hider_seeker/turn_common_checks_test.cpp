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
        state.hiders[hiderId].server_id = NO_SERVER_ID;
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

// A match with hider 0 holding one pending move (move_id 1), so every entry
// point would otherwise have something to act on.
MatchState makeMatchWithPendingMove() {
    MatchState state = makeActiveMatch(2);
    state.tick = 12;
    state.revision = 40;
    CountingPolicy policy;
    queueMove(state, hiderAction(1, 0, HiderActionMode::PAY_GOAL), policy);
    return state;
}

} // namespace

TEST_CASE("TURN-008 every entry point rejects MATCH_NOT_ACTIVE outside PHASE_ACTIVE", "[hider_seeker][turn]") {
    for (uint8_t phase : {static_cast<uint8_t>(PHASE_LOBBY), static_cast<uint8_t>(PHASE_COMPLETE)}) {
        MatchState state = makeMatchWithPendingMove();
        REQUIRE(state.pending_count == 1);
        state.phase = phase;
        state.hiders[1].action_points = 10;
        CountingPolicy policy;
        const std::vector<uint8_t> before = serialize(state);

        const TurnResult played = playMove(state, hiderAction(2, 1, HiderActionMode::BOOST_CPU), policy);
        REQUIRE(played.status == MoveStatus::REJECTED);
        REQUIRE(played.reason == TurnReason::MATCH_NOT_ACTIVE);

        const TurnResult queued = queueMove(state, hiderAction(3, 1, HiderActionMode::BOOST_CPU), policy);
        REQUIRE(queued.status == MoveStatus::REJECTED);
        REQUIRE(queued.reason == TurnReason::MATCH_NOT_ACTIVE);

        const TurnResult passed = passTicks(state, 3, policy);
        REQUIRE(passed.status == MoveStatus::REJECTED);
        REQUIRE(passed.reason == TurnReason::MATCH_NOT_ACTIVE);

        const TurnResult cancelled = cancelPendingMove(state, 0, 1);
        REQUIRE(cancelled.status == MoveStatus::REJECTED);
        REQUIRE(cancelled.reason == TurnReason::MATCH_NOT_ACTIVE);

        REQUIRE(policy.calls == 0);
        REQUIRE(serialize(state) == before);
    }
}

TEST_CASE("TURN-008 a captured hider's calls are rejected HIDER_CAPTURED", "[hider_seeker][turn]") {
    MatchState state = makeMatchWithPendingMove();
    state.hiders[0].status = HIDER_STATUS_CAPTURED;
    state.hiders[0].action_points = 10;
    CountingPolicy policy;
    const std::vector<uint8_t> before = serialize(state);

    const TurnResult played = playMove(state, hiderAction(2, 0, HiderActionMode::BOOST_CPU), policy);
    REQUIRE(played.status == MoveStatus::REJECTED);
    REQUIRE(played.reason == TurnReason::HIDER_CAPTURED);

    const TurnResult queued = queueMove(state, hiderAction(3, 0, HiderActionMode::BOOST_CPU), policy);
    REQUIRE(queued.status == MoveStatus::REJECTED);
    REQUIRE(queued.reason == TurnReason::HIDER_CAPTURED);

    const TurnResult cancelled = cancelPendingMove(state, 0, 1);
    REQUIRE(cancelled.status == MoveStatus::REJECTED);
    REQUIRE(cancelled.reason == TurnReason::HIDER_CAPTURED);

    REQUIRE(policy.calls == 0);
    REQUIRE(serialize(state) == before);
}

TEST_CASE("TURN-008 inactive or out-of-range hiders are rejected RULE_REJECTED", "[hider_seeker][turn]") {
    MatchState state = makeMatchWithPendingMove();
    // Hider 2 exists in the array but is inactive; MAX_HIDERS is out of range
    REQUIRE(state.hiders[2].active == 0);
    CountingPolicy policy;
    const std::vector<uint8_t> before = serialize(state);

    for (uint8_t hiderId : {static_cast<uint8_t>(2), MAX_HIDERS}) {
        const TurnResult played = playMove(state, hiderAction(2, hiderId, HiderActionMode::BOOST_CPU), policy);
        REQUIRE(played.status == MoveStatus::REJECTED);
        REQUIRE(played.reason == TurnReason::RULE_REJECTED);
        REQUIRE(played.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);

        const TurnResult queued = queueMove(state, hiderAction(3, hiderId, HiderActionMode::BOOST_CPU), policy);
        REQUIRE(queued.status == MoveStatus::REJECTED);
        REQUIRE(queued.reason == TurnReason::RULE_REJECTED);
        REQUIRE(queued.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);

        const TurnResult cancelled = cancelPendingMove(state, hiderId, 1);
        REQUIRE(cancelled.status == MoveStatus::REJECTED);
        REQUIRE(cancelled.reason == TurnReason::RULE_REJECTED);
        REQUIRE(cancelled.ruleOutcome == RuleOutcome::REJECTED_INVALID_TARGET);
    }

    REQUIRE(policy.calls == 0);
    REQUIRE(serialize(state) == before);
}

TEST_CASE("TURN-008 every non-REJECTED call increases revision by exactly 1", "[hider_seeker][turn]") {
    MatchState state = makeActiveMatch(2);
    state.revision = 100;
    state.hiders[1].action_points = 5;
    CountingPolicy policy;

    // QUEUED
    REQUIRE(queueMove(state, hiderAction(1, 0, HiderActionMode::CONFIGURE_SERVER), policy).status == MoveStatus::QUEUED);
    REQUIRE(state.revision == 101);

    // APPLIED immediately
    REQUIRE(playMove(state, hiderAction(2, 1, HiderActionMode::BOOST_CPU), policy).status == MoveStatus::APPLIED);
    REQUIRE(state.revision == 102);

    // passTicks over several ticks still bumps revision once
    REQUIRE(passTicks(state, 2, policy).status == MoveStatus::APPLIED);
    REQUIRE(state.revision == 103);

    // CANCELLED
    REQUIRE(cancelPendingMove(state, 0, 1).status == MoveStatus::CANCELLED);
    REQUIRE(state.revision == 104);

    // APPLIED after a multi-tick advance
    const TurnResult advanced = playMove(state, hiderAction(3, 0, HiderActionMode::PAY_GOAL), policy);
    REQUIRE(advanced.status == MoveStatus::APPLIED);
    REQUIRE(advanced.ticksAdvanced > 1);
    REQUIRE(state.revision == 105);

    // A REJECTED call leaves it alone
    REQUIRE(passTicks(state, 0, policy).status == MoveStatus::REJECTED);
    REQUIRE(state.revision == 105);
}
