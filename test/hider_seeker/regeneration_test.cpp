#include <catch2/catch_test_macros.hpp>
#include "state.hpp"
#include "regeneration.hpp"
#include "hider_status.hpp"
#include "loa_rules.hpp"

using namespace hider_seeker;

TEST_CASE("regenerateForTick AP regen and cap", "[regeneration]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.hider_count = 3;
    state.seeker.connection_tokens = 5;
    state.seeker.max_tokens = 10;
    
    // Set up hiders with different action points
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 3;
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 8;
    
    state.hiders[2].active = true;
    state.hiders[2].status = HIDER_STATUS_CAPTURED; // Captured hider should not gain AP
    state.hiders[2].action_points = 5;
    
    // Call regeneration function
    regenerateForTick(state);
    
    // Check that hiders gained AP (2 each) but capped at 10
    REQUIRE(state.hiders[0].action_points == 5); // 3 + 2 = 5
    REQUIRE(state.hiders[1].action_points == 10); // 8 + 2 = 10 (capped)
    REQUIRE(state.hiders[2].action_points == 5); // Captured hider should not gain AP
    
    // Check that seeker tokens are unchanged (no longer regenerate) // 5a3 seeker tokens no longer regenerate
    REQUIRE(state.seeker.connection_tokens == 5);
}

TEST_CASE("regenerateForTick inactive hider gains nothing", "[regeneration]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.hider_count = 2;
    state.seeker.connection_tokens = 3;
    state.seeker.max_tokens = 10;

    // Set up hiders - one inactive, one active
    state.hiders[0].active = false; // Inactive hider should not gain AP
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5;
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 3;
    
    // Call regeneration function
    regenerateForTick(state);
    
    // Check that only active hider gained AP
    REQUIRE(state.hiders[0].action_points == 5); // Inactive hider should not gain AP
    REQUIRE(state.hiders[1].action_points == 5); // Active hider gains 2 AP
    
    // Check that seeker tokens are unchanged (no longer regenerate) // 5a3 seeker tokens no longer regenerate
    REQUIRE(state.seeker.connection_tokens == 3);
}

TEST_CASE("regenerateForTick LOA unchanged", "[regeneration]") {
    MatchState state{};
    
    // Initialize a basic state
    state.schema_version = 2;
    state.hider_count = 2;
    
    // Set up hiders with different resources
    state.hiders[0].active = true;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 5;
    state.hiders[0].resources[0] = 100; // Some resource
    
    state.hiders[1].active = true;
    state.hiders[1].status = HIDER_STATUS_ACTIVE;
    state.hiders[1].action_points = 3;
    state.hiders[1].resources[1] = 200; // Another resource
    
    // Store original values
    uint16_t orig_resource_0 = state.hiders[0].resources[0];
    uint16_t orig_resource_1 = state.hiders[1].resources[1];
    
    // Call regeneration function
    regenerateForTick(state);
    
    // Check that LOA (resources) are unchanged
    REQUIRE(state.hiders[0].resources[0] == orig_resource_0);
    REQUIRE(state.hiders[1].resources[1] == orig_resource_1);
    REQUIRE(state.hiders[0].resources[LOA_RESOURCE_INDEX] == 100);
}

TEST_CASE("regenerateForTick AP rises by 2 per call and caps at 10", "[regeneration]") {
    MatchState state{};
    state.hiders[0].active = 1;
    state.hiders[0].status = HIDER_STATUS_ACTIVE;
    state.hiders[0].action_points = 0;

    const uint8_t expected[] = {2, 4, 6, 8, 10, 10, 10};
    for (uint8_t value : expected) {
        regenerateForTick(state);
        REQUIRE(state.hiders[0].action_points == value);
    }
    REQUIRE(PLACEHOLDER_AP_REGEN_PER_TICK == 2);
    REQUIRE(PLACEHOLDER_AP_CAP == 10);
}

TEST_CASE("regenerateForTick leaves seeker tokens unchanged", "[regeneration]") {
    MatchState state{};
    state.seeker.max_tokens = 5;
    state.seeker.connection_tokens = 2;

    regenerateForTick(state);
    REQUIRE(state.seeker.connection_tokens == 2); // 5a3: seeker tokens no longer regenerate
    regenerateForTick(state);
    REQUIRE(state.seeker.connection_tokens == 2); // 5a3: seeker tokens no longer regenerate
    regenerateForTick(state);
    REQUIRE(state.seeker.connection_tokens == 2); // 5a3: seeker tokens no longer regenerate
}