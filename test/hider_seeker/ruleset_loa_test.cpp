#include <catch2/catch_test_macros.hpp>
#include "loa_rules.hpp"
#include "lifecycle_events.hpp"
#include "state.hpp"

using namespace hider_seeker;

namespace {
// Scratch LOA-005 output channel for tests that do not inspect emitted events.
Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> scratchEvents;
uint8_t scratchEventCount = 0;

RuleOutcome addServer(MatchState& match, uint8_t hiderId) {
    scratchEventCount = 0;
    return applyAddServer(match, hiderId, scratchEvents, scratchEventCount);
}

RuleOutcome removeServer(MatchState& match, uint8_t hiderId, uint8_t serverId) {
    scratchEventCount = 0;
    return applyRemoveServer(match, hiderId, serverId, scratchEvents, scratchEventCount);
}
} // namespace

TEST_CASE("applyAddServer spends exactly 1 LOA and activates a free slot", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 2 LOA
    match.hiders[0].resources[0] = 2; // Assuming resource type 0 is LOA
    
    // Add server should succeed
    RuleOutcome outcome = addServer(match, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);

    // Check that 1 LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 1);
    
    // Check that a slot was activated
    REQUIRE(match.servers[0].active == 1);
    REQUIRE(match.servers[0].owner_hider_id == 0);
    REQUIRE(match.server_count == 1);
}

TEST_CASE("applyAddServer is rejected with zero LOA spent when no slot is free", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 1 LOA
    match.hiders[0].resources[0] = 1;
    
    // Fill all servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        match.servers[i].active = 1;
        match.server_count++;
    }
    
    // Add server should fail
    RuleOutcome outcome = addServer(match, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_NO_SLOT_AVAILABLE);
    
    // Check that no LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 1);
}

TEST_CASE("applyAddServer is rejected with zero LOA spent when LOA < 1", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 0 LOA
    match.hiders[0].resources[0] = 0;
    
    // Add server should fail
    RuleOutcome outcome = addServer(match, 0);
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    
    // Check that no LOA was spent
    REQUIRE(match.hiders[0].resources[0] == 0);
}

TEST_CASE("applyRemoveServer atomically relocates an occupant or is fully rejected", "[loa]") {
    MatchState match = {};
    
    // Initialize a hider with 2 LOA
    match.hiders[0].resources[0] = 2;
    
    // Add server first
    addServer(match, 0);
    REQUIRE(match.hiders[0].resources[0] == 1);

    // Remove server should succeed (no active hider occupies the slot, so no relocation needed)
    RuleOutcome outcome = removeServer(match, 0, 0);
    REQUIRE(outcome == RuleOutcome::ACCEPTED);

    // Check that the removal itself spent exactly 1 LOA (REMOVE_SERVER costs 1 LOA per design §4.2)
    REQUIRE(match.hiders[0].resources[0] == 0);
    
    // Check that slot was deactivated
    REQUIRE(match.servers[0].active == 0);
    REQUIRE(match.server_count == 0);
}

TEST_CASE("applyRemoveServer on an occupied slot relocates atomically or is fully rejected", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].resources[0] = 3;

    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED); // slot 0
    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED); // slot 1
    match.hiders[0].server_id = 0; // hider occupies slot 0
    REQUIRE(match.hiders[0].resources[0] == 1);
    REQUIRE(match.server_count == 2);
    REQUIRE(match.event_count == 0); // lifecycle events are not wired into MatchState::events

    SECTION("no other active server to relocate to rejects with no state change") {
        resetServerSlot(match.servers[1]);
        match.server_count = 1;
        Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
        uint8_t eventCount = 0;
        REQUIRE(applyRemoveServer(match, 0, 0, events, eventCount) == RuleOutcome::REJECTED_RELOCATION_FAILED);
        REQUIRE(eventCount == 0);
        REQUIRE(match.hiders[0].resources[0] == 1);
        REQUIRE(match.servers[0].active == 1);
        REQUIRE(match.servers[0].owner_hider_id == 0);
        REQUIRE(match.server_count == 1);
        REQUIRE(match.hiders[0].server_id == 0);
        REQUIRE(match.event_count == 0);
    }

    SECTION("occupant is relocated to the internally computed active server in the same action") {
        REQUIRE(removeServer(match, 0, 0) == RuleOutcome::ACCEPTED);
        REQUIRE(match.hiders[0].resources[0] == 0);
        REQUIRE(match.servers[0].active == 0);
        REQUIRE(match.servers[0].owner_hider_id == 0xFF);
        REQUIRE(match.servers[1].active == 1);
        REQUIRE(match.server_count == 1);
        REQUIRE(match.hiders[0].server_id == 1);
        REQUIRE(match.event_count == 0);
    }

    SECTION("non-owning hider can remove server with sufficient LOA (LOA-002 acceptance criteria)") {
        // Fixture: hider 0 owns slots 0 and 1 and occupies slot 0.
        // Hider 1 owns nothing but has sufficient LOA.
        match.hiders[1].active = 1;
        match.hiders[1].server_id = 1;
        match.hiders[1].resources[LOA_RESOURCE_INDEX] = 2;
        REQUIRE(match.servers[0].owner_hider_id == 0);

        // Non-owning hider 1 removes hider 0's slot 0 (LOA-002).
        REQUIRE(removeServer(match, 1, 0) == RuleOutcome::ACCEPTED);

        // Only the submitting hider pays the LOA cost.
        REQUIRE(match.hiders[1].resources[LOA_RESOURCE_INDEX] == 1);
        REQUIRE(match.hiders[0].resources[LOA_RESOURCE_INDEX] == 1);

        // The slot is removed and its occupant relocated to the remaining active slot.
        REQUIRE(match.servers[0].active == 0);
        REQUIRE(match.servers[0].owner_hider_id == 0xFF);
        REQUIRE(match.servers[1].active == 1);
        REQUIRE(match.server_count == 1);
        REQUIRE(match.hiders[0].server_id == 1);
        REQUIRE(match.hiders[1].server_id == 1);
    }
}

TEST_CASE("applyAddServer does not write lifecycle events into MatchState::events", "[loa]") {
    MatchState match = {};
    match.tick = 42;
    match.hiders[0].resources[0] = 1;

    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED);
    REQUIRE(match.event_count == 0);
    REQUIRE(match.event_head == 0);
}

TEST_CASE("applyAddServer/applyRemoveServer expose the public lifecycle event to the caller (LOA-005)", "[loa]") {
    MatchState match = {};
    match.tick = 42;
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 2;
    Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS> events;
    uint8_t eventCount = 0;
    const PublicLifecycleEvent fallback = {HiderSeekerEventType::SERVER_ADDED, 0};

    REQUIRE(applyAddServer(match, 0, events, eventCount) == RuleOutcome::ACCEPTED);
    REQUIRE(eventCount == 1);
    REQUIRE(events.getOrDefault(0, fallback).type == HiderSeekerEventType::SERVER_ADDED);
    REQUIRE(events.getOrDefault(0, fallback).tick == 42);

    match.tick = 43;
    REQUIRE(applyRemoveServer(match, 0, 0, events, eventCount) == RuleOutcome::ACCEPTED);
    REQUIRE(eventCount == 2);
    REQUIRE(events.getOrDefault(1, fallback).type == HiderSeekerEventType::SERVER_REMOVED);
    REQUIRE(events.getOrDefault(1, fallback).tick == 43);

    // A rejected action appends nothing.
    REQUIRE(applyAddServer(match, 0, events, eventCount) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(eventCount == 2);
}

TEST_CASE("applyRemoveServer invalidates every reference to the removed slot (LOA-004)", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 3;
    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED); // slot 0
    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED); // slot 1
    match.hiders[0].server_id = 1;

    // Goal 0: configured-server requirement on slot 0 -> must be cancelled.
    match.goals[0].active = 1;
    match.goals[0].status = 3;
    match.goals[0].requirement_count = 2;
    match.goals[0].requirements[0] = {REQ_RESOURCE_MIN, 2, 5, 0};
    match.goals[0].requirements[1] = {REQ_CONFIGURED_SERVER, 0, 0, 0};
    match.hiders[0].goal_progress[0] = 7;
    // Goal 1: metric-threshold requirement on slot 1 -> untouched.
    match.goals[1].active = 1;
    match.goals[1].status = 3;
    match.goals[1].requirement_count = 1;
    match.goals[1].requirements[0] = {REQ_SERVER_METRIC_THRESHOLD, 1, 0, 50};
    match.hiders[0].goal_progress[1] = 4;
    // Goal 2: non-server requirement whose subject_id happens to be 0 -> untouched.
    match.goals[2].active = 1;
    match.goals[2].status = 3;
    match.goals[2].requirement_count = 1;
    match.goals[2].requirements[0] = {REQ_RESOURCE_SPEND, 0, 2, 0};

    // Pending seeker targets (oldest -> newest): 0, 1, 0, 2, with head at 14 (wraps).
    match.seeker.target_head = 14;
    match.seeker.target_ids[14] = 0;
    match.seeker.target_ids[15] = 1;
    match.seeker.target_ids[0] = 0;
    match.seeker.target_ids[1] = 2;
    match.seeker.target_count = 4;

    match.seeker.belief[0] = 9;
    match.seeker.last_probe_tick[0] = 5;
    match.seeker.belief[1] = 6;
    match.seeker.last_probe_tick[1] = 4;

    REQUIRE(removeServer(match, 0, 0) == RuleOutcome::ACCEPTED);

    // Goal referencing the removed slot is cancelled and its progress reset.
    REQUIRE(match.goals[0].active == 0);
    REQUIRE(match.goals[0].status == GOAL_STATUS_CANCELLED_SERVER_REMOVED);
    REQUIRE(match.hiders[0].goal_progress[0] == 0);
    // Goals not referencing the removed slot are unchanged.
    REQUIRE(match.goals[1].active == 1);
    REQUIRE(match.goals[1].status == 3);
    REQUIRE(match.hiders[0].goal_progress[1] == 4);
    REQUIRE(match.goals[2].active == 1);
    REQUIRE(match.goals[2].status == 3);

    // Pending targets for slot 0 are removed; survivors keep order from the head.
    REQUIRE(match.seeker.target_count == 2);
    REQUIRE(match.seeker.target_head == 14);
    REQUIRE(match.seeker.target_ids[14] == 1);
    REQUIRE(match.seeker.target_ids[15] == 2);

    // Seeker knowledge of slot 0 is reset; slot 1 is untouched.
    REQUIRE(match.seeker.belief[0] == 0);
    REQUIRE(match.seeker.last_probe_tick[0] == 0);
    REQUIRE(match.seeker.belief[1] == 6);
    REQUIRE(match.seeker.last_probe_tick[1] == 4);

    // A still-pending REMOVE_SERVER intent naming the removed slot is invalidated
    // at apply time, with no LOA spent.
    REQUIRE(match.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 1;
    REQUIRE(removeServer(match, 0, 0) == RuleOutcome::REJECTED_INVALID_TARGET);
    REQUIRE(match.hiders[0].resources[LOA_RESOURCE_INDEX] == 1);
}

TEST_CASE("resetServerSlot clears metrics/history/ownership/trace fields", "[loa]") {
    MatchState match = {};
    
    // Set up a server with data
    match.servers[0].active = 1;
    match.servers[0].owner_hider_id = 5;
    match.servers[0].trace_strength = 100;
    match.servers[0].trace_age = 20;
    match.servers[0].current.cpu = 50;
    match.servers[0].history[0].cpu = 40;
    match.servers[0].history_head = 1;
    
    // Reset the slot
    resetServerSlot(match.servers[0]);
    
    // Check that all fields are cleared
    REQUIRE(match.servers[0].active == 0);
    REQUIRE(match.servers[0].owner_hider_id == 0xFF);
    REQUIRE(match.servers[0].trace_strength == 0);
    REQUIRE(match.servers[0].trace_age == 0);
    REQUIRE(match.servers[0].current.cpu == 0);
    REQUIRE(match.servers[0].history[0].cpu == 0);
    REQUIRE(match.servers[0].history_head == 0);
}

TEST_CASE("invalidateServerReferences refunds seeker token when seeker was connected", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID;
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 2;
    
    // Add a server to get slot 0
    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED);
    
    // Connect the seeker to server 0 (set bit in mask)
    match.seeker_connection_mask |= (1u << 0);
    match.seeker.connection_tokens = 4;  // Start with 4 tokens
    
    // Remove server 0 - should refund 1 token
    REQUIRE(removeServer(match, 0, 0) == RuleOutcome::ACCEPTED);
    
    // Check that the seeker got a token back and the bit was cleared
    REQUIRE(match.seeker.connection_tokens == 5);  // 4 + 1 = 5
    REQUIRE((match.seeker_connection_mask & (1u << 0)) == 0);  // Bit should be cleared
}

TEST_CASE("invalidateServerReferences makes no changes when seeker was not connected", "[loa]") {
    MatchState match = {};
    match.hiders[0].active = 1;
    match.hiders[0].server_id = NO_SERVER_ID;
    match.hiders[0].resources[LOA_RESOURCE_INDEX] = 2;
    
    // Add a server to get slot 0
    REQUIRE(addServer(match, 0) == RuleOutcome::ACCEPTED);
    
    // Do NOT connect the seeker to server 0 (bit not set in mask)
    match.seeker.connection_tokens = 4;  // Start with 4 tokens
    
    // Remove server 0 - should make no changes
    REQUIRE(removeServer(match, 0, 0) == RuleOutcome::ACCEPTED);
    
    // Check that the seeker tokens and mask remain unchanged
    REQUIRE(match.seeker.connection_tokens == 4);  // Should still be 4
    REQUIRE((match.seeker_connection_mask & (1u << 0)) == 0);  // Bit should still be cleared
}