#include <catch2/catch_test_macros.hpp>
#include "rng.hpp"
#include <type_traits>

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
    // 1. The only draw API cannot be invoked on a const state or an rvalue copy,
    //    so there is no way to "peek" at a draw without consuming it.
    static_assert(std::is_invocable_r_v<uint32_t, decltype(&nextRandomValue), RngState&>,
                  "nextRandomValue must draw from a mutable RngState");
    static_assert(!std::is_invocable_v<decltype(&nextRandomValue), const RngState&>,
                  "nextRandomValue must not accept a const RngState (would allow peeking)");
    static_assert(!std::is_invocable_v<decltype(&nextRandomValue), RngState&&>,
                  "nextRandomValue must not accept a temporary RngState (would allow peeking)");

    // 2. Every draw consumes exactly one step: state changes and counter advances
    //    by one, so a revealed value can never be drawn again from the advanced state.
    RngState rng = {12345, 0};
    uint32_t previousValue = 0;
    for (uint64_t i = 0; i < 32; ++i) {
        const RngState before = rng;
        const uint32_t value = nextRandomValue(rng);
        REQUIRE(rng.state != before.state);
        REQUIRE(rng.counter == before.counter + 1);

        // Re-drawing from the advanced state does not reproduce the revealed value.
        RngState replay = rng;
        REQUIRE(nextRandomValue(replay) != value);

        if (i > 0) {
            REQUIRE(value != previousValue);
        }
        previousValue = value;
    }
}