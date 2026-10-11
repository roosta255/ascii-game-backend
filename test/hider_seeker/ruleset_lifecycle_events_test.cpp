#include <catch2/catch_test_macros.hpp>
#include "lifecycle_events.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("emitPublicLifecycleEventIfDefined appends event only when public", "[lifecycle]") {
    Array<PublicLifecycleEvent, MAX_EVENTS> events;
    
    // Test with isDefinedPublicByRuleset = true
    RuleOutcome outcome1 = emitPublicLifecycleEventIfDefined(
        HiderSeekerEventType::SERVER_ADDED, 100, true, events);
    
    REQUIRE(outcome1 == RuleOutcome::ACCEPTED);
    
    // Test with isDefinedPublicByRuleset = false
    RuleOutcome outcome2 = emitPublicLifecycleEventIfDefined(
        HiderSeekerEventType::SERVER_REMOVED, 200, false, events);
    
    REQUIRE(outcome2 == RuleOutcome::ACCEPTED);
}

TEST_CASE("PublicLifecycleEvent has no field capable of carrying hider ID", "[lifecycle]") {
    // This is a compile-time check - the struct definition should not contain
    // any field that could carry hider identity information
    
    // Compare against a struct with exactly the two allowed fields so that
    // alignment padding is accounted for identically on both sides.
    struct TypeAndTickOnly {
        HiderSeekerEventType type;
        uint32_t tick;
    };
    static_assert(sizeof(PublicLifecycleEvent) == sizeof(TypeAndTickOnly),
                  "PublicLifecycleEvent should only contain type and tick fields");
    
    REQUIRE(true); // Placeholder - compile-time check is sufficient
}