#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../../src/hider_seeker/ruleset/connection_rules.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"
#include "../../../src/hider_seeker/tick/tick_types.hpp"

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
    state.hiders[0].server_id = NO_SERVER_ID;  // Explicitly set to ensure disconnected
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
    state.hiders[0].server_id = NO_SERVER_ID;  // Explicitly set to ensure disconnected
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

// CONN-004 (wagers) tests
TEST_CASE("applyWager - basic test with one hider", "[connection]") {
    MatchState match{};
    match.tick = 0x1234;
    match.seeker.connection_tokens = 10;
    match.hider_connection_pool = 5;
    match.seeker_connection_mask = (1u << 2) | (1u << 4); // Connected to servers 2 and 4
    
    // Set up hiders: hider 0 on server 0, hider 1 and 3 on server 2
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0;
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 2;
    match.hiders[3].active = 1;
    match.hiders[3].server_id = 2;
    
    TickOutput output{};
    
    // Test: applyWager(match, 2, 1, output) → hider 1 disconnected, hider 3 still on 2
    applyWager(match, 2, 1, output);
    
    REQUIRE(match.hiders[1].server_id == NO_SERVER_ID); // hider 1 disconnected
    REQUIRE(match.hiders[3].server_id == 2); // hider 3 still connected
    REQUIRE(match.seeker.connection_tokens == 10); // tokens unchanged (no loss)
    REQUIRE(match.seeker.belief[2] == 1); // belief updated
    REQUIRE(match.seeker.last_probe_tick[2] == 0x34); // low byte of tick
    
    // Check public event
    REQUIRE(output.publicEventCount == 1);
    output.publicEvents.accessConst(0, [](const PublicLifecycleEvent& event) {
        REQUIRE(event.type == HiderSeekerEventType::SERVER_PROBED);
        REQUIRE(event.server_id == 2);
        REQUIRE(event.outcome == PROBE_OUTCOME_HIT);
    });
    
    // Check wager record
    REQUIRE(output.wagerCount == 1);
    output.wagers.accessConst(0, [](const WagerRecord& wager) {
        REQUIRE(wager.serverId == 2);
        REQUIRE(wager.wager == 1);
        REQUIRE(wager.destroyed == 1);
        REQUIRE(wager.lost == 0);
    });
    
    // Check hider pool unchanged
    REQUIRE(match.hider_connection_pool == 5);
    
    // Check seeker mask unchanged
    REQUIRE(match.seeker_connection_mask == (1u << 2) | (1u << 4));
}

TEST_CASE("applyWager - test with multiple hiders", "[connection]") {
    MatchState match{};
    match.tick = 0x1234;
    match.seeker.connection_tokens = 10;
    match.hider_connection_pool = 5;
    match.seeker_connection_mask = (1u << 2) | (1u << 4); // Connected to servers 2 and 4
    
    // Set up hiders: hider 0 on server 0, hider 1 and 3 on server 2
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0;
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 2;
    match.hiders[3].active = 1;
    match.hiders[3].server_id = 2;
    
    TickOutput output{};
    
    // Test: applyWager(match, 2, 3, output) → hiders 1 and 3 disconnected
    applyWager(match, 2, 3, output);
    
    REQUIRE(match.hiders[1].server_id == NO_SERVER_ID); // hider 1 disconnected
    REQUIRE(match.hiders[3].server_id == NO_SERVER_ID); // hider 3 disconnected
    REQUIRE(match.seeker.connection_tokens == 9); // tokens reduced by 1 (lost = 3 - 2)
    REQUIRE(match.seeker.belief[2] == 2); // belief updated
    
    // Check public event
    REQUIRE(output.publicEventCount == 1);
    output.publicEvents.accessConst(0, [](const PublicLifecycleEvent& event) {
        REQUIRE(event.type == HiderSeekerEventType::SERVER_PROBED);
        REQUIRE(event.server_id == 2);
        REQUIRE(event.outcome == PROBE_OUTCOME_HIT);
    });
    
    // Check wager record
    REQUIRE(output.wagerCount == 1);
    output.wagers.accessConst(0, [](const WagerRecord& wager) {
        REQUIRE(wager.serverId == 2);
        REQUIRE(wager.wager == 3);
        REQUIRE(wager.destroyed == 2);
        REQUIRE(wager.lost == 1);
    });
    
    // Check hider pool unchanged
    REQUIRE(match.hider_connection_pool == 5);
    
    // Check seeker mask unchanged
    REQUIRE(match.seeker_connection_mask == (1u << 2) | (1u << 4));
}

TEST_CASE("applyWager - test with no hiders on server", "[connection]") {
    MatchState match{};
    match.tick = 0x1234;
    match.seeker.connection_tokens = 10;
    match.hider_connection_pool = 5;
    match.seeker_connection_mask = (1u << 2) | (1u << 4); // Connected to servers 2 and 4
    
    // Set up hiders: hider 0 on server 0, hider 1 and 3 on server 2
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0;
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 2;
    match.hiders[3].active = 1;
    match.hiders[3].server_id = 2;
    
    TickOutput output{};
    
    // Test: applyWager(match, 4, 2, output) on a server with no hiders
    applyWager(match, 4, 2, output);
    
    REQUIRE(match.seeker.connection_tokens == 8); // tokens reduced by 2 (lost = 2 - 0)
    REQUIRE(match.seeker.belief[4] == 0); // belief updated
    
    // Check public event
    REQUIRE(output.publicEventCount == 1);
    output.publicEvents.accessConst(0, [](const PublicLifecycleEvent& event) {
        REQUIRE(event.type == HiderSeekerEventType::SERVER_PROBED);
        REQUIRE(event.server_id == 4);
        REQUIRE(event.outcome == PROBE_OUTCOME_MISS);
    });
    
    // Check wager record
    REQUIRE(output.wagerCount == 1);
    output.wagers.accessConst(0, [](const WagerRecord& wager) {
        REQUIRE(wager.serverId == 4);
        REQUIRE(wager.wager == 2);
        REQUIRE(wager.destroyed == 0);
        REQUIRE(wager.lost == 2);
    });
    
    // Check seeker mask unchanged
    REQUIRE(match.seeker_connection_mask == (1u << 2) | (1u << 4));
}

TEST_CASE("applyWager - test with capped wager", "[connection]") {
    MatchState match{};
    match.tick = 0x1234;
    match.seeker.connection_tokens = 3;
    match.hider_connection_pool = 5;
    match.seeker_connection_mask = (1u << 2) | (1u << 4); // Connected to servers 2 and 4
    
    // Set up hiders: hider 0 on server 0, hider 1 and 3 on server 2
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0;
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 2;
    match.hiders[3].active = 1;
    match.hiders[3].server_id = 2;
    
    TickOutput output{};
    
    // Test: applyWager(match, 4, 5, output) - wager capped at 3
    applyWager(match, 4, 5, output);
    
    REQUIRE(match.seeker.connection_tokens == 0); // tokens reduced to 0
    REQUIRE(match.seeker.belief[4] == 0); // belief updated
    
    // Check wager record
    REQUIRE(output.wagerCount == 1);
    output.wagers.accessConst(0, [](const WagerRecord& wager) {
        REQUIRE(wager.serverId == 4);
        REQUIRE(wager.wager == 3);
        REQUIRE(wager.destroyed == 0);
        REQUIRE(wager.lost == 3);
    });
    
    // Check seeker mask unchanged
    REQUIRE(match.seeker_connection_mask == (1u << 2) | (1u << 4));
}

TEST_CASE("applyWager - test with zero tokens", "[connection]") {
    MatchState match{};
    match.tick = 0x1234;
    match.seeker.connection_tokens = 0;
    match.hider_connection_pool = 5;
    match.seeker_connection_mask = (1u << 2) | (1u << 4); // Connected to servers 2 and 4
    
    // Set up hiders: hider 0 on server 0, hider 1 and 3 on server 2
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0;
    match.hiders[1].active = 1;
    match.hiders[1].server_id = 2;
    match.hiders[3].active = 1;
    match.hiders[3].server_id = 2;
    
    TickOutput output{};
    
    // Test: applyWager(match, 4, 5, output) - zero tokens
    applyWager(match, 4, 5, output);
    
    REQUIRE(match.seeker.connection_tokens == 0); // tokens unchanged
    REQUIRE(match.seeker.belief[4] == 0); // belief unchanged
    
    // Check no events produced
    REQUIRE(output.publicEventCount == 0);
    REQUIRE(output.wagerCount == 0);
    
    // Check seeker mask unchanged
    REQUIRE(match.seeker_connection_mask == (1u << 2) | (1u << 4));
}