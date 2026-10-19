#include <catch2/catch_test_macros.hpp>
#include "service_test_support.hpp"
#include "state.hpp"
#include "match_end.hpp"
#include "../hider_seeker/ruleset/rule_types.hpp"

using namespace hider_seeker;
using namespace hs_test;

TEST_CASE("ServiceMatchStartTest", "[service][match-start]") {
    ServiceFixture fixture;
    
    // Setup: create a 2-hider match, join alice and bob, start it, with no
    // CONNECT steps, so the state is exactly what startMatch left.
    fixture.setupTwoHidersDisconnected();
    
    // Load the stored MatchState after start
    auto record = fixture.record();
    const MatchState& state = record.state;
    
    // CONN-002a: Check that hider_connection_pool is set correctly (3 connections per hider)
    REQUIRE(state.hider_connection_pool == 6);  // 3 * 2 hiders = 6
    
    // CONN-002a: Check that seeker_connection_mask is initialized to 0
    REQUIRE(state.seeker_connection_mask == 0);
    
    // CONN-002a: Check that winner is initialized to NONE
    REQUIRE(state.winner == static_cast<uint8_t>(MatchWinner::NONE));
    
    // CONN-002c: Check that all hiders start disconnected (server_id = NO_SERVER_ID)
    REQUIRE(state.hiders[0].server_id == NO_SERVER_ID);
    REQUIRE(state.hiders[1].server_id == NO_SERVER_ID);
    
    // CONN-002b: Check that seeker connection tokens are initialized to 12
    REQUIRE(state.seeker.connection_tokens == 12);
    REQUIRE(state.seeker.max_tokens == 12);
}