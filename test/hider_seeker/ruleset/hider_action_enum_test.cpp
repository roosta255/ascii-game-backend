#include <catch2/catch_test_macros.hpp>
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"

using namespace hider_seeker;

// This test verifies that the HiderAction.enum contains the exact CONNECT/DISCONNECT lines
// and that the static asserts for their enum values compile.

TEST_CASE("HiderAction.enum contains CONNECT and DISCONNECT", "[hider_action]") {
    // The test is mainly to ensure the enum definition compiles correctly
    // with the new CONNECT and DISCONNECT entries.
    
    // Verify that the enum values are as expected (these should compile due to static_asserts)
    static_assert(static_cast<uint8_t>(HiderActionMode::CONNECT) == 7);
    static_assert(static_cast<uint8_t>(HiderActionMode::DISCONNECT) == 8);
    
    // The actual enum definition is in HiderAction.enum which is processed by the build system
    // This test just ensures that the static asserts compile correctly
    REQUIRE(true); // Placeholder - the real check is compilation
}

TEST_CASE("HiderAction.enum has correct ordering", "[hider_action]") {
    // Verify that existing actions still have their expected indices
    static_assert(static_cast<uint8_t>(HiderActionMode::ADD_SERVER) == 0);
    static_assert(static_cast<uint8_t>(HiderActionMode::REMOVE_SERVER) == 1);
    static_assert(static_cast<uint8_t>(HiderActionMode::CONFIGURE_SERVER) == 2);
    static_assert(static_cast<uint8_t>(HiderActionMode::SUPPRESS_ERROR) == 3);
    static_assert(static_cast<uint8_t>(HiderActionMode::BOOST_CPU) == 4);
    static_assert(static_cast<uint8_t>(HiderActionMode::ROTATE_ROUTE) == 5);
    static_assert(static_cast<uint8_t>(HiderActionMode::PAY_GOAL) == 6);
    static_assert(static_cast<uint8_t>(HiderActionMode::CONNECT) == 7);
    static_assert(static_cast<uint8_t>(HiderActionMode::DISCONNECT) == 8);
    
    REQUIRE(true); // Placeholder - the real check is compilation
}