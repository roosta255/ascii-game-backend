#include <catch2/catch_test_macros.hpp>
#include "lifecycle_events.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("emitPublicLifecycleEventIfDefined appends event only when public", "[lifecycle]") {
    Array<PublicLifecycleEvent, MAX_EVENTS> events;
    uint8_t count = 0;
    const PublicLifecycleEvent fallback = {HiderSeekerEventType::SERVER_ADDED, 0};

    // Test with isDefinedPublicByRuleset = true
    RuleOutcome outcome1 = emitPublicLifecycleEventIfDefined(
        HiderSeekerEventType::SERVER_ADDED, 100, true, events, count);

    REQUIRE(outcome1 == RuleOutcome::ACCEPTED);
    REQUIRE(count == 1);
    REQUIRE(events.getOrDefault(0, fallback).type == HiderSeekerEventType::SERVER_ADDED);
    REQUIRE(events.getOrDefault(0, fallback).tick == 100);

    // Test with isDefinedPublicByRuleset = false: nothing appended
    RuleOutcome outcome2 = emitPublicLifecycleEventIfDefined(
        HiderSeekerEventType::SERVER_REMOVED, 200, false, events, count);

    REQUIRE(outcome2 == RuleOutcome::ACCEPTED);
    REQUIRE(count == 1);
}

TEST_CASE("emitPublicLifecycleEventIfDefined appends at the next free slot without overwriting", "[lifecycle]") {
    Array<PublicLifecycleEvent, MAX_EVENTS> events;
    uint8_t count = 0;
    const PublicLifecycleEvent fallback = {HiderSeekerEventType::SERVER_ADDED, 0};

    REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_ADDED, 7, true, events, count)
            == RuleOutcome::ACCEPTED);
    REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_REMOVED, 7, true, events, count)
            == RuleOutcome::ACCEPTED);
    REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_ADDED, 8, true, events, count)
            == RuleOutcome::ACCEPTED);

    REQUIRE(count == 3);
    REQUIRE(events.getOrDefault(0, fallback).type == HiderSeekerEventType::SERVER_ADDED);
    REQUIRE(events.getOrDefault(0, fallback).tick == 7);
    REQUIRE(events.getOrDefault(1, fallback).type == HiderSeekerEventType::SERVER_REMOVED);
    REQUIRE(events.getOrDefault(1, fallback).tick == 7);
    REQUIRE(events.getOrDefault(2, fallback).type == HiderSeekerEventType::SERVER_ADDED);
    REQUIRE(events.getOrDefault(2, fallback).tick == 8);
}

TEST_CASE("emitPublicLifecycleEventIfDefined appends nothing when the array is full", "[lifecycle]") {
    Array<PublicLifecycleEvent, MAX_EVENTS> events;
    uint8_t count = 0;
    for (int i = 0; i < MAX_EVENTS; ++i) {
        REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_ADDED,
                                                  static_cast<uint32_t>(i), true, events, count)
                == RuleOutcome::ACCEPTED);
    }
    REQUIRE(count == MAX_EVENTS);

    REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_REMOVED, 999, true, events, count)
            == RuleOutcome::REJECTED_NO_SLOT_AVAILABLE);
    REQUIRE(count == MAX_EVENTS);
    const PublicLifecycleEvent fallback = {HiderSeekerEventType::SERVER_ADDED, 0};
    REQUIRE(events.getOrDefault(MAX_EVENTS - 1, fallback).tick == static_cast<uint32_t>(MAX_EVENTS - 1));
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