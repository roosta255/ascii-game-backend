#include <catch2/catch_test_macros.hpp>
#include "intent_ordering.hpp"
#include "action_rules.hpp"
#include "hider_status.hpp"
#include "loa_rules.hpp"
#include "state.hpp"

using namespace hider_seeker;

TEST_CASE("orderAcceptedIntents sorts by receiptSequence", "[ordering]") {
    Array<IntentRecord, MAX_INTENTS_PER_TICK> intents;
    
    // Set up some intent records with different sequence numbers
    IntentRecord record1 = {10, 100, 0, HiderActionMode::ADD_SERVER};
    IntentRecord record2 = {5, 200, 1, HiderActionMode::REMOVE_SERVER};
    IntentRecord record3 = {15, 300, 2, HiderActionMode::CONFIGURE_SERVER};
    
    // Remaining slots are value-initialized (receiptSequence == 0) and sort first
    const int base = intents.size() - 3;
    intents.set(base + 0, record1);
    intents.set(base + 1, record2);
    intents.set(base + 2, record3);

    // Sort the intents
    orderAcceptedIntents(intents);

    // Check that they are sorted by receiptSequence (ascending)
    const IntentRecord fallback = {0, 0, 0, HiderActionMode::ADD_SERVER};
    REQUIRE(intents.getOrDefault(base + 0, fallback).receiptSequence == 5);
    REQUIRE(intents.getOrDefault(base + 1, fallback).receiptSequence == 10);
    REQUIRE(intents.getOrDefault(base + 2, fallback).receiptSequence == 15);
}

TEST_CASE("isIntentAlreadyApplied correctly rejects already applied keys", "[ordering]") {
    Array<uint64_t, MAX_INTENTS_PER_TICK> consumedKeys;
    
    // Mark some keys as consumed
    consumedKeys.set(0, 100);
    consumedKeys.set(1, 200);
    consumedKeys.set(2, 300);
    const int consumedCount = 3;

    // Check that already applied keys are rejected
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 100) == true);
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 200) == true);
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 300) == true);

    // Check that new keys are not rejected
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 400) == false);
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 500) == false);

    // Unused slots must never be mistaken for a legitimate key of 0
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount, 0) == false);

    // A genuinely consumed key of 0 is detected
    consumedKeys.set(3, 0);
    REQUIRE(isIntentAlreadyApplied(consumedKeys, consumedCount + 1, 0) == true);
}

TEST_CASE("No function mutates MatchState::tick on AP/token exhaustion", "[ordering]") {
    MatchState match = {};
    match.tick = 100;
    match.servers[0].active = 1;
    match.servers[0].trace_strength = 100;

    SECTION("hider AP exhaustion: actions are rejected and tick is unchanged") {
        match.hiders[0].action_points = 0;
        const HiderActionMode modes[] = {
            HiderActionMode::CONFIGURE_SERVER, HiderActionMode::SUPPRESS_ERROR,
            HiderActionMode::BOOST_CPU, HiderActionMode::ROTATE_ROUTE,
            HiderActionMode::PAY_GOAL};
        for (HiderActionMode mode : modes) {
            REQUIRE(applyHiderAction(match, 0, mode) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
            REQUIRE(match.tick == 100);
        }
    }

    SECTION("ADD_SERVER/REMOVE_SERVER charge no AP (their cost is 1 LOA only)") {
        match.hiders[0].action_points = 0;
        REQUIRE(applyHiderAction(match, 0, HiderActionMode::ADD_SERVER) == RuleOutcome::ACCEPTED);
        REQUIRE(applyHiderAction(match, 0, HiderActionMode::REMOVE_SERVER) == RuleOutcome::ACCEPTED);
        REQUIRE(match.hiders[0].action_points == 0);
        REQUIRE(match.tick == 100);
    }

    SECTION("hider AP drained to zero by accepted actions leaves tick unchanged") {
        match.hiders[0].action_points = 2; // exactly one BOOST_CPU (placeholder cost 2)
        REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::ACCEPTED);
        REQUIRE(match.hiders[0].action_points == 0);
        REQUIRE(match.tick == 100);
        REQUIRE(applyHiderAction(match, 0, HiderActionMode::BOOST_CPU) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
        REQUIRE(match.tick == 100);
    }

    SECTION("LOA exhaustion: add/remove server are rejected and tick is unchanged") {
        match.hiders[0].resources[0] = 0;
        match.servers[0].owner_hider_id = 0;
        Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
        uint8_t eventCount = 0;
        REQUIRE(applyAddServer(match, 0, events, eventCount) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
        REQUIRE(applyRemoveServer(match, 0, 0, events, eventCount) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
        REQUIRE(eventCount == 0);
        REQUIRE(match.tick == 100);
    }
}