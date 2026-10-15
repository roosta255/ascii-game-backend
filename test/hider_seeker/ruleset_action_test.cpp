#include <catch2/catch_test_macros.hpp>
#include "action_rules.hpp"
#include "server_metrics.hpp"
#include "state.hpp"
#include <string>

using namespace hider_seeker;

TEST_CASE("applyHiderAction uses backend table for cost/effect and deducts only on acceptance", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with 10 AP
    match.hiders[0].action_points = 10;
    
    // Apply an action - should succeed
    RuleOutcome outcome = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    
    // Check that AP was deducted (placeholder value of 5)
    REQUIRE(match.hiders[0].action_points == 5);
}

TEST_CASE("applyHiderAction does not deduct AP on rejection", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with 2 AP (less than the placeholder cost of 5)
    match.hiders[0].action_points = 2;
    
    // Apply an action - should fail due to insufficient AP
    RuleOutcome outcome = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    
    // Check that AP was not deducted
    REQUIRE(match.hiders[0].action_points == 2);
}

TEST_CASE("applyHiderAction always takes cost/effect from backend table", "[action]") {
    MatchState match = {};
    
    // Initialize a hider with sufficient AP for all actions
    match.hiders[0].action_points = 20;
    
    // Test different action modes - all should use backend table values
    RuleOutcome outcome1 = applyHiderAction(match, 0, HiderActionMode::CONFIGURE_SERVER);
    REQUIRE(outcome1 == RuleOutcome::ACCEPTED);
    
    RuleOutcome outcome2 = applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR);
    REQUIRE(outcome2 == RuleOutcome::ACCEPTED);
    
    RuleOutcome outcome3 = applyHiderAction(match, 0, HiderActionMode::BOOST_CPU);
    REQUIRE(outcome3 == RuleOutcome::ACCEPTED);
    
    // Check that AP was deducted for each action (using placeholder values)
    // CONFIGURE_SERVER: 5, SUPPRESS_ERROR: 3, BOOST_CPU: 2
    REQUIRE(match.hiders[0].action_points == 20 - 5 - 3 - 2); // Should be 10
}

TEST_CASE("applyHiderAction applies the table-defined effect to the hider's occupied server", "[action]") {
    MatchState match = {};
    match.hiders[0].action_points = 20;
    match.hiders[0].server_id = 2;
    match.servers[2].active = 1;
    match.servers[2].current.cpu = 50;
    match.servers[2].current.errors = 4;
    match.servers[2].current.network = 250;
    const uint8_t serverCountBefore = match.server_count;

    // BOOST_CPU: placeholder +15 cpu
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.cpu == 65);

    // SUPPRESS_ERROR: placeholder -10 errors, clamped at 0
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.errors == 0);

    // ROTATE_ROUTE: placeholder +5 network, clamped to NETWORK's max (DEF-008)
    REQUIRE(applyHiderAction(match, 0, HiderActionMode::ROTATE_ROUTE) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[2].current.network == 100); // 250 + 5 = 255, clamped to NETWORK max 100

    // Effects never touch structural state such as server_count
    REQUIRE(match.server_count == serverCountBefore);
}

TEST_CASE("applyHiderAction applies no effect when rejected", "[action]") {
    MatchState match = {};
    match.hiders[0].action_points = 1; // below BOOST_CPU placeholder cost of 2
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].current.cpu = 50;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(match.servers[0].current.cpu == 50);
    REQUIRE(match.hiders[0].action_points == 1);
}

TEST_CASE("HiderAction.enum generates seven modes with unchanged indices", "[action][def]") {
    REQUIRE(HIDER_ACTION_MODE_COUNT == 7);
    REQUIRE(static_cast<int>(HiderActionMode::ADD_SERVER) == 0);
    REQUIRE(static_cast<int>(HiderActionMode::REMOVE_SERVER) == 1);
    REQUIRE(static_cast<int>(HiderActionMode::CONFIGURE_SERVER) == 2);
    REQUIRE(static_cast<int>(HiderActionMode::SUPPRESS_ERROR) == 3);
    REQUIRE(static_cast<int>(HiderActionMode::BOOST_CPU) == 4);
    REQUIRE(static_cast<int>(HiderActionMode::ROTATE_ROUTE) == 5);
    REQUIRE(static_cast<int>(HiderActionMode::PAY_GOAL) == 6);
}

TEST_CASE("hiderActionName and hiderActionFromName round-trip every mode", "[action][def]") {
    for (int index = 0; index < HIDER_ACTION_MODE_COUNT; ++index) {
        const HiderActionMode mode = static_cast<HiderActionMode>(index);
        const char* name = hiderActionName(mode);
        REQUIRE(name != nullptr);
        HiderActionMode parsed = static_cast<HiderActionMode>(HIDER_ACTION_MODE_COUNT);
        REQUIRE(hiderActionFromName(name, parsed));
        REQUIRE(parsed == mode);
    }
    REQUIRE(std::string(hiderActionName(HiderActionMode::BOOST_CPU)) == "BOOST_CPU");
    REQUIRE(hiderActionName(static_cast<HiderActionMode>(HIDER_ACTION_MODE_COUNT)) == nullptr);
}

TEST_CASE("hiderActionFromName accepts exact names only", "[action][def]") {
    HiderActionMode parsed = HiderActionMode::PAY_GOAL;
    REQUIRE_FALSE(hiderActionFromName("", parsed));
    REQUIRE_FALSE(hiderActionFromName("boost_cpu", parsed));
    REQUIRE_FALSE(hiderActionFromName("PASS", parsed));
    REQUIRE_FALSE(hiderActionFromName("BOOST", parsed));
    REQUIRE(parsed == HiderActionMode::PAY_GOAL); // untouched on failure
}

TEST_CASE("hiderActionDef returns the HiderAction.enum cost and effects", "[action][def]") {
    const HiderActionDef& suppress = hiderActionDef(HiderActionMode::SUPPRESS_ERROR);
    REQUIRE(suppress.ap_cost == 3);
    REQUIRE(suppress.effect_count == 2);
    ActionEffect first{};
    ActionEffect second{};
    suppress.effects.accessConst(0, [&first](const ActionEffect& effect) { first = effect; });
    suppress.effects.accessConst(1, [&second](const ActionEffect& effect) { second = effect; });
    REQUIRE(first.metric == SERVER_METRIC_ERRORS);
    REQUIRE(first.delta == -10);
    REQUIRE(second.metric == SERVER_METRIC_TRACE);
    REQUIRE(second.delta == 20);

    const HiderActionDef& payGoal = hiderActionDef(HiderActionMode::PAY_GOAL);
    REQUIRE(payGoal.ap_cost == 10);
    REQUIRE(payGoal.effect_count == 0);
}

TEST_CASE("applyHiderAction BOOST_CPU raises cpu and trace and resets trace_age", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 10;
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].current.cpu = 50;
    match.servers[0].trace_strength = 0;
    match.servers[0].trace_age = 7;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[0].current.cpu == 65);   // 50 + 15
    REQUIRE(match.servers[0].trace_strength == 5); // 0 + 5
    REQUIRE(match.servers[0].trace_age == 0);      // trace rose, so reset from 7
    REQUIRE(match.hiders[0].action_points == 8);   // 10 - 2
}

TEST_CASE("applyHiderAction SUPPRESS_ERROR clamps errors at the metric min", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 10;
    match.hiders[0].server_id = 1;
    match.servers[1].active = 1;
    match.servers[1].current.errors = 4;
    match.servers[1].trace_strength = 10;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[1].current.errors == 0);  // 4 - 10 = -6, clamped to ERRORS min 0
    REQUIRE(match.servers[1].trace_strength == 30); // 10 + 20
    REQUIRE(match.hiders[0].action_points == 7);    // 10 - 3
}

TEST_CASE("applyHiderAction ROTATE_ROUTE clamps network at the metric max", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 10;
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].current.network = 98;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::ROTATE_ROUTE) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[0].current.network == 100); // 98 + 5 = 103, clamped to NETWORK max 100 (no longer 255)
    REQUIRE(match.servers[0].trace_strength == 10);   // 0 + 10
}

TEST_CASE("applyHiderAction PAY_GOAL leaves trace_age unchanged", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 10;
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].trace_strength = 9;
    match.servers[0].trace_age = 4;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::PAY_GOAL) == RuleOutcome::ACCEPTED);
    REQUIRE(match.servers[0].trace_strength == 9);
    REQUIRE(match.servers[0].trace_age == 4);
    REQUIRE(match.hiders[0].action_points == 0); // 10 - 10
}

TEST_CASE("applyHiderAction rejected for insufficient AP changes no metric, trace, or trace_age", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 2; // below SUPPRESS_ERROR's cost of 3
    match.hiders[0].server_id = 0;
    match.servers[0].active = 1;
    match.servers[0].current.errors = 40;
    match.servers[0].trace_strength = 6;
    match.servers[0].trace_age = 3;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::SUPPRESS_ERROR) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(match.servers[0].current.errors == 40);
    REQUIRE(match.servers[0].trace_strength == 6);
    REQUIRE(match.servers[0].trace_age == 3);
    REQUIRE(match.hiders[0].action_points == 2);
}

TEST_CASE("applyHiderAction with an inactive occupied server pays AP only", "[action][def]") {
    MatchState match = {};
    match.hiders[0].action_points = 10;
    match.hiders[0].server_id = 3;
    match.servers[3].active = 0;
    match.servers[3].current.cpu = 50;
    match.servers[3].trace_strength = 0;
    match.servers[3].trace_age = 7;

    REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::ACCEPTED);
    REQUIRE(match.hiders[0].action_points == 8); // 10 - 2
    REQUIRE(match.servers[3].current.cpu == 50);
    REQUIRE(match.servers[3].trace_strength == 0);
    REQUIRE(match.servers[3].trace_age == 7);
}