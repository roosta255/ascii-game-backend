#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../../src/hider_seeker/ruleset/connection_rules.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"

using namespace hider_seeker;

TEST_CASE("applyConnect rejects invalid hider", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.servers[0].active = true;
    state.hider_connection_pool = 1;
    
    RuleOutcome outcome = applyConnect(state, 100, 0); // Invalid hider ID
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyConnect rejects invalid server", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.servers[0].active = false; // Inactive server
    state.hider_connection_pool = 1;
    
    RuleOutcome outcome = applyConnect(state, 0, 0); // Invalid server ID (inactive)
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyConnect rejects already connected hider", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.hiders[0].server_id = 5; // Already connected
    state.servers[0].active = true;
    state.hider_connection_pool = 1;
    
    RuleOutcome outcome = applyConnect(state, 0, 0); // Already connected hider
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyConnect rejects when connection pool is empty", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.servers[0].active = true;
    state.hider_connection_pool = 0; // Empty pool
    
    RuleOutcome outcome = applyConnect(state, 0, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_NO_CONNECTIONS);
}

TEST_CASE("applyConnect succeeds with valid inputs", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.hiders[0].action_points = 10;
    state.servers[0].active = true;
    state.hider_connection_pool = 1;
    
    RuleOutcome outcome = applyConnect(state, 0, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].server_id == 0);
    REQUIRE(state.hider_connection_pool == 0);
    REQUIRE(state.hiders[0].action_points == 10); // No cost for connect
}

TEST_CASE("applyDisconnect rejects invalid hider", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = false; // Inactive hider
    
    RuleOutcome outcome = applyDisconnect(state, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyDisconnect rejects not connected hider", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.hiders[0].server_id = NO_SERVER_ID; // Not connected
    
    RuleOutcome outcome = applyDisconnect(state, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_INVALID_TARGET);
}

TEST_CASE("applyDisconnect succeeds with valid inputs", "[connection]") {
    MatchState state{};
    // Set up a basic valid state
    state.hiders[0].active = true;
    state.hiders[0].server_id = 5;
    state.hiders[0].action_points = 10;
    
    RuleOutcome outcome = applyDisconnect(state, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(state.hiders[0].server_id == NO_SERVER_ID);
    REQUIRE(state.hider_connection_pool == 1); // Incremented
    REQUIRE(state.hiders[0].action_points == 10); // No cost for disconnect
}