#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"

using namespace hider_seeker;

// CONN-010: Verify enum values for the new hider actions.
TEST_CASE("HiderActionMode_enum_values_are_correct", "[action][conn-010]") {
    // Verify that existing actions retain their indices
    REQUIRE(static_cast<uint8_t>(HiderActionMode::ADD_SERVER) == 0);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::REMOVE_SERVER) == 1);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::CONFIGURE_SERVER) == 2);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::SUPPRESS_ERROR) == 3);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::BOOST_CPU) == 4);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::ROTATE_ROUTE) == 5);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::PAY_GOAL) == 6);
    
    // CONN-010: Verify new hider actions have correct indices
    REQUIRE(static_cast<uint8_t>(HiderActionMode::CONNECT) == 7);
    REQUIRE(static_cast<uint8_t>(HiderActionMode::DISCONNECT) == 8);
}

// CONN-010: Test static_asserts for the new hider actions
TEST_CASE("HiderActionMode_static_asserts_for_new_actions", "[action][conn-010]") {
    // These are already in action_rules.hpp, but let's make sure they compile
    static_assert(static_cast<uint8_t>(HiderActionMode::CONNECT) == 7);
    static_assert(static_cast<uint8_t>(HiderActionMode::DISCONNECT) == 8);
}