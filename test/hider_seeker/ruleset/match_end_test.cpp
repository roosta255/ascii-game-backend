#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/ruleset/match_end.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"
#include "../../../src/hider_seeker/ruleset/rule_types.hpp"

using namespace hider_seeker;

TEST_CASE("checkMatchEnd - hider loss only", "[match_end]") {
    MatchState match{};
    match.phase = PHASE_ACTIVE;
    match.winner = static_cast<uint8_t>(MatchWinner::NONE);
    match.hider_connection_pool = 0;
    
    // Set up hiders: one hider active, but not connected to any server
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID; // This hider is not connected
    
    match.seeker.connection_tokens = 5;
    
    checkMatchEnd(match);
    
    REQUIRE(match.phase == PHASE_COMPLETE);
    REQUIRE(match.winner == static_cast<uint8_t>(MatchWinner::SEEKER));
}

TEST_CASE("checkMatchEnd - seeker loss only", "[match_end]") {
    MatchState match{};
    match.phase = PHASE_ACTIVE;
    match.winner = static_cast<uint8_t>(MatchWinner::NONE);
    match.hider_connection_pool = 3;
    
    // Set up hiders: one hider active, but not connected to any server
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID; // This hider is not connected
    
    match.seeker.connection_tokens = 0;
    match.seeker_connection_mask = 0;
    
    checkMatchEnd(match);
    
    REQUIRE(match.phase == PHASE_COMPLETE);
    REQUIRE(match.winner == static_cast<uint8_t>(MatchWinner::HIDERS));
}

TEST_CASE("checkMatchEnd - both loss conditions true", "[match_end]") {
    MatchState match{};
    match.phase = PHASE_ACTIVE;
    match.winner = static_cast<uint8_t>(MatchWinner::NONE);
    match.hider_connection_pool = 0;
    
    // Set up hiders: one hider active, but not connected to any server
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID; // This hider is not connected
    
    match.seeker.connection_tokens = 0;
    match.seeker_connection_mask = 0;
    
    checkMatchEnd(match);
    
    REQUIRE(match.phase == PHASE_COMPLETE);
    REQUIRE(match.winner == static_cast<uint8_t>(MatchWinner::SEEKER));
}

TEST_CASE("checkMatchEnd - hider loss condition false", "[match_end]") {
    MatchState match{};
    match.phase = PHASE_ACTIVE;
    match.winner = static_cast<uint8_t>(MatchWinner::NONE);
    match.hider_connection_pool = 0;
    
    // Set up hiders: one hider active and connected to server 0
    match.hiders[0].active = 1;
    match.hiders[0].server_id = 0; // This hider is connected to server 0
    
    match.seeker.connection_tokens = 5;
    
    checkMatchEnd(match);
    
    REQUIRE(match.phase == PHASE_ACTIVE);
    REQUIRE(match.winner == static_cast<uint8_t>(MatchWinner::NONE));
}

TEST_CASE("checkMatchEnd - non-active phase", "[match_end]") {
    MatchState match{};
    match.phase = PHASE_LOBBY; // Not PHASE_ACTIVE
    match.winner = static_cast<uint8_t>(MatchWinner::NONE);
    match.hider_connection_pool = 0;
    
    // Set up hiders: one hider active, but not connected to any server
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID; // This hider is not connected
    
    match.seeker.connection_tokens = 0;
    match.seeker_connection_mask = 0;
    
    checkMatchEnd(match);
    
    REQUIRE(match.phase == PHASE_LOBBY); // Should remain unchanged
    REQUIRE(match.winner == static_cast<uint8_t>(MatchWinner::NONE)); // Should remain unchanged
}