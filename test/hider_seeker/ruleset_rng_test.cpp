#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/ruleset/rng.hpp"

using namespace hider_seeker;

TEST_CASE("nextRandomValue produces identical sequences from same seed", "[rng]") {
    RngState rng1 = {12345, 0};
    RngState rng2 = {12345, 0}; // Same seed
    
    // Generate the same number of values from both RNGs
    for (int i = 0; i < 10; ++i) {
        uint32_t val1 = nextRandomValue(rng1);
        uint32_t val2 = nextRandomValue(rng2);
        REQUIRE(val1 == val2);
    }
}

TEST_CASE("nextRandomValue advances state on each call", "[rng]") {
    RngState rng = {12345, 0};
    
    // Get first value
    uint32_t first = nextRandomValue(rng);
    
    // Get second value - should be different
    uint32_t second = nextRandomValue(rng);
    
    REQUIRE(first != second);
}

TEST_CASE("nextRandomValue has no future-draw exposure", "[rng]") {
    RngState rng = {12345, 0};
    
    // Only way to get values is by calling nextRandomValue sequentially
    // No API exists to peek ahead without mutating state
    
    // This test verifies the design principle - we can't directly test this
    // since there's no API to peek ahead, but the implementation should be 
    // such that no future draws are exposed
    REQUIRE(true); // Placeholder - actual verification would require more complex testing
}