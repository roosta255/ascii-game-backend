#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "hider_seeker/domain/state.hpp"
#include "hider_seeker/domain/serialization.hpp"
#include "hider_seeker/ruleset/match_end.hpp"

using namespace hider_seeker;

TEST_CASE("SerializationV3Test", "[serialization]") {
    // Test 1: Round-trip serialization with new fields
    MatchState state;
    state.schema_version = 3;
    state.hider_connection_pool = 7;
    state.seeker_connection_mask = 0x8001;
    state.winner = static_cast<uint8_t>(MatchWinner::SEEKER);
    
    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);
    
    MatchState loaded_state;
    bool success = loadMatchState(buffer, loaded_state);
    
    REQUIRE(success == true);
    REQUIRE(loaded_state.hider_connection_pool == 7);
    REQUIRE(loaded_state.seeker_connection_mask == 0x8001);
    REQUIRE(loaded_state.winner == static_cast<uint8_t>(MatchWinner::SEEKER));
    
    // Test 2: Loading a version-2 buffer should fail
    buffer.clear();
    // Write schema version 2 marker
    buffer.push_back(2); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);
    // Write a minimal valid v2 buffer (just enough to pass the header check)
    buffer.push_back(2); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // schema_version
    buffer.push_back(1); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // ruleset_version
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // revision
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // tick
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // rng_state
    buffer.push_back(0);  // phase
    buffer.push_back(0);  // hider_count
    buffer.push_back(0);  // server_count
    
    success = loadMatchState(buffer, loaded_state);
    REQUIRE(success == false);  // Should fail because version 2 is not supported
    
    // Test 3: Loading a version-3 buffer with invalid winner value should fail
    buffer.clear();
    // Write schema version 3 marker
    buffer.push_back(3); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);
    // Write the rest of a valid v3 buffer
    buffer.push_back(3); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // schema_version
    buffer.push_back(1); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // ruleset_version
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // revision
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // tick
    buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0); buffer.push_back(0);  // rng_state
    buffer.push_back(0);  // phase
    buffer.push_back(0);  // hider_count
    buffer.push_back(0);  // server_count
    
    // Add the required fields for a valid v3 buffer
    for (int i = 0; i < MAX_HIDERS * (4 + RESOURCE_TYPE_COUNT * 2 + MAX_GOALS) + 
                 MAX_SERVERS * (5 + 7 + HISTORY_TICKS * 7 + 1) +
                 (5 + 2 * MAX_SERVERS + MAX_SEEKER_TARGETS) +
                 MAX_GOALS * (10 + MAX_REQUIREMENTS_PER_GOAL * 6) +
                 MAX_EVENTS * 5 +
                 2 + 
                 MAX_PENDING_MOVES * (4 + 4 + 1 + 1 + 1 + 1 + 1) + 
                 1 + 1 + 4; i++) {
        buffer.push_back(0);
    }
    
    // Set the new fields with invalid winner value
    buffer[buffer.size() - 3] = 3;  // winner = 3 (invalid)
    
    success = loadMatchState(buffer, loaded_state);
    REQUIRE(success == false);  // Should fail because winner > 2
}