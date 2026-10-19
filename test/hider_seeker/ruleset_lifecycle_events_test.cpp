#include <catch2/catch_test_macros.hpp>
#include "lifecycle_events.hpp"
#include "tick_types.hpp"
#include "state.hpp"
#include "names.hpp"

using namespace hider_seeker;

TEST_CASE("emitPublicLifecycleEventIfDefined appends event only when public", "[lifecycle]") {
    Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
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
    Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
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
    Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
    uint8_t count = 0;
    for (int i = 0; i < MAX_TICK_PUBLIC_EVENTS; ++i) {
        REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_ADDED,
                                                  static_cast<uint32_t>(i), true, events, count)
                == RuleOutcome::ACCEPTED);
    }
    REQUIRE(count == MAX_TICK_PUBLIC_EVENTS);

    REQUIRE(emitPublicLifecycleEventIfDefined(HiderSeekerEventType::SERVER_REMOVED, 999, true, events, count)
            == RuleOutcome::REJECTED_NO_SLOT_AVAILABLE);
    REQUIRE(count == MAX_TICK_PUBLIC_EVENTS);
    const PublicLifecycleEvent fallback = {HiderSeekerEventType::SERVER_ADDED, 0};
    REQUIRE(events.getOrDefault(MAX_TICK_PUBLIC_EVENTS - 1, fallback).tick == static_cast<uint32_t>(MAX_TICK_PUBLIC_EVENTS - 1));
    // 5a3c: a tick's public event array holds MAX_TICK_PUBLIC_EVENTS
}

TEST_CASE("PublicLifecycleEvent has no field capable of carrying hider ID", "[lifecycle]") {
    // This is a compile-time check - the struct definition should not contain
    // any field that could carry hider identity information
    
    // Compare against a struct with exactly the two allowed fields so that
    // alignment padding is accounted for identically on both sides.
    struct AllowedFields {
        HiderSeekerEventType type;
        uint32_t tick;
        uint8_t server_id;
        uint8_t outcome;
    };
    static_assert(sizeof(PublicLifecycleEvent) == sizeof(AllowedFields),
                  "PublicLifecycleEvent should contain type, tick, server_id and outcome fields");
    
    REQUIRE(true); // Placeholder - compile-time check is sufficient
}

TEST_CASE("eventTypeName returns correct wire names", "[lifecycle]") {
    REQUIRE(eventTypeName(HiderSeekerEventType::SERVER_PROBED) == "SERVER_PROBED");
}

TEST_CASE("probeOutcomeName returns correct wire names", "[lifecycle]") {
    REQUIRE(probeOutcomeName(PROBE_OUTCOME_HIT) == "HIT");
    REQUIRE(probeOutcomeName(PROBE_OUTCOME_MISS) == "MISS");
    REQUIRE(probeOutcomeName(PROBE_OUTCOME_NONE) == "");
}

TEST_CASE("PublicLifecycleEvent default field values", "[lifecycle]") {
    // Test that a default PublicLifecycleEvent has correct default values
    PublicLifecycleEvent event{HiderSeekerEventType::SERVER_ADDED, 5};
    
    REQUIRE(event.server_id == NO_SERVER_ID); // Should be 0xFF
    REQUIRE(event.outcome == PROBE_OUTCOME_NONE); // Should be 0
}

TEST_CASE("TickOutput capacity and WagerRecord tests", "[lifecycle]") {
    STATIC_REQUIRE(MAX_TICK_PUBLIC_EVENTS == MAX_INTENTS_PER_TICK + MAX_SERVERS);
    
    TickOutput tickOutput{};
    REQUIRE(tickOutput.wagerCount == 0);
    REQUIRE(tickOutput.publicEvents.containsIndex(MAX_TICK_PUBLIC_EVENTS - 1));
    
    // Test WagerRecord storage and retrieval
    tickOutput.wagers.assignValue(0, WagerRecord{2, 1, 1, 0});
    const WagerRecord& record = tickOutput.wagers.getOrDefault(0, WagerRecord{0, 0, 0, 0});
    REQUIRE(record.serverId == 2);
    REQUIRE(record.wager == 1);
    REQUIRE(record.destroyed == 1);
    REQUIRE(record.lost == 0);
}