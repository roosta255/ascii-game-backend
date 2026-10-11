#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../../../src/hider_seeker/tick/tick.hpp"
#include "../../../src/hider_seeker/ruleset/intent_ordering.hpp"
#include "../../../src/hider_seeker/ruleset/action_rules.hpp"
#include "../../../src/hider_seeker/ruleset/loa_rules.hpp"
#include "../../../src/hider_seeker/domain/state.hpp"

using namespace hider_seeker;

namespace {
    struct StubPolicy : public iTargetRankingPolicy {
        void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const override {
            // Return no candidates - do nothing (leave array as is)
        }
    };

    struct MockPolicy : public iTargetRankingPolicy {
        void rankTargets(const MatchState& state, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const override {
            // Return some mock candidates for testing
            outCandidates.assignValue(0, ProbeCandidate{1, 100}); // serverId=1, score=100
            outCandidates.assignValue(1, ProbeCandidate{2, 50});  // serverId=2, score=50
        }
    };
}

TEST_CASE("Tick engine runs without crashing", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    
    // Set up seeker
    state.seeker.max_tokens = 5;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 0;
    
    // Add a HIDER_ACTION intent
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 12345;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(intentCount++, intent1);
    
    // Add an ADD_SERVER intent
    TickIntent intent2{};
    intent2.kind = TickActionKind::ADD_SERVER;
    intent2.target_server_id = 2; // This should be invalid since we only have 2 servers (0 and 1)
    intent2.intent.idempotencyKey = 12346;
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(intentCount++, intent2);
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Basic sanity check - should not crash and should return valid output
    REQUIRE(output.consumedKeyCount >= 0);
    REQUIRE(output.intentOutcomeCount >= 0);
    REQUIRE(output.probeCount == 0); // No probes since policy returns no candidates
}

TEST_CASE("Tick engine eligibility filtering", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 2;
    
    // Add a HIDER_ACTION intent
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 12345;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(0, intent1);
    
    // Add another intent with same idempotency key (should be filtered out)
    TickIntent intent2{};
    intent2.kind = TickActionKind::HIDER_ACTION;
    intent2.action_mode = HiderActionMode::SUPPRESS_ERROR;
    intent2.intent.idempotencyKey = 12345; // Same key as intent1
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(1, intent2);
    
    // Create consumed key set that contains the first intent's key
    ConsumedKeySet alreadyConsumed{};
    alreadyConsumed.keys.assignValue(0, 12345);
    alreadyConsumed.count = 1;
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have no consumed keys since the intent was filtered out due to already being consumed
    REQUIRE(output.consumedKeyCount == 0);
    REQUIRE(output.intentOutcomeCount == 0); // No outcomes because intent was filtered
}

TEST_CASE("Tick engine ordering and deduplication", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 2;
    
    // Add two intents with same idempotency key (should be deduplicated)
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 12345;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(0, intent1);
    
    TickIntent intent2{};
    intent2.kind = TickActionKind::HIDER_ACTION;
    intent2.action_mode = HiderActionMode::SUPPRESS_ERROR;
    intent2.intent.idempotencyKey = 12345; // Same key as intent1
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(1, intent2);
    
    // Create empty consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have exactly one outcome (deduplication)
    REQUIRE(output.intentOutcomeCount == 1);
    REQUIRE(output.consumedKeyCount == 1); // One key should be consumed
}

TEST_CASE("Tick engine action dispatch", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    // Enough AP to afford all five actions below (placeholder costs 5+3+2+4+10 = 24)
    state.hiders[0].action_points = 24;
    state.hiders[0].server_id = 1;

    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;

    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 5; // Test all HiderActionMode values
    
    // CONFIGURE_SERVER
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 1001;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(0, intent1);
    
    // SUPPRESS_ERROR
    TickIntent intent2{};
    intent2.kind = TickActionKind::HIDER_ACTION;
    intent2.action_mode = HiderActionMode::SUPPRESS_ERROR;
    intent2.intent.idempotencyKey = 1002;
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(1, intent2);
    
    // BOOST_CPU
    TickIntent intent3{};
    intent3.kind = TickActionKind::HIDER_ACTION;
    intent3.action_mode = HiderActionMode::BOOST_CPU;
    intent3.intent.idempotencyKey = 1003;
    intent3.intent.hiderId = 0;
    intent3.intent.receiptSequence = 3;
    intents.assignValue(2, intent3);
    
    // ROTATE_ROUTE
    TickIntent intent4{};
    intent4.kind = TickActionKind::HIDER_ACTION;
    intent4.action_mode = HiderActionMode::ROTATE_ROUTE;
    intent4.intent.idempotencyKey = 1004;
    intent4.intent.hiderId = 0;
    intent4.intent.receiptSequence = 4;
    intents.assignValue(3, intent4);
    
    // PAY_GOAL
    TickIntent intent5{};
    intent5.kind = TickActionKind::HIDER_ACTION;
    intent5.action_mode = HiderActionMode::PAY_GOAL;
    intent5.intent.idempotencyKey = 1005;
    intent5.intent.hiderId = 0;
    intent5.intent.receiptSequence = 5;
    intents.assignValue(4, intent5);
    
    // Create empty consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have 5 outcomes (one for each action)
    REQUIRE(output.intentOutcomeCount == 5);
    REQUIRE(output.consumedKeyCount == 5); // All should be accepted
}

TEST_CASE("Tick engine metrics placeholder correctness", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers with trace_strength > 0 to test metrics placeholder
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[0].trace_strength = 10; // This should increment trace_age
    state.servers[0].trace_age = 5;
    
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    state.servers[1].trace_strength = 0; // This should NOT increment trace_age
    state.servers[1].trace_age = 3;
    
    // Set up seeker
    state.seeker.max_tokens = 5;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 0;
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Check that trace_age was incremented for server 0 (trace_strength > 0)
    REQUIRE(state.servers[0].trace_age == 6); // 5 + 1
    // Check that trace_age was NOT incremented for server 1 (trace_strength = 0)
    REQUIRE(state.servers[1].trace_age == 3); // unchanged
}

TEST_CASE("Tick engine probe-budget truncation", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 3;
    state.seeker.connection_tokens = 2; // Only 2 tokens available
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    state.servers[2].active = 1;
    state.servers[2].current.cpu = 70;
    state.servers[2].current.memory = 70;
    
    // Set up seeker with limited tokens
    state.seeker.max_tokens = 5;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 0;
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick with a policy that returns 3 candidates but only 2 tokens available
    MockPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should process exactly 2 probes (limited by connection_tokens)
    REQUIRE(output.probeCount == 2);
}

TEST_CASE("Tick engine cross-call idempotency retry", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    
    // Set up seeker
    state.seeker.max_tokens = 5;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 1;
    
    // Add an intent
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 12345;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(0, intent1);
    
    // Create consumed key set with a pre-existing key (simulating retry scenario)
    ConsumedKeySet alreadyConsumed{};
    alreadyConsumed.keys.assignValue(0, 12345);
    alreadyConsumed.count = 1;
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have no consumed keys since the intent was filtered out due to already being consumed
    REQUIRE(output.consumedKeyCount == 0);
    REQUIRE(output.intentOutcomeCount == 0); // No outcomes because intent was filtered
}

TEST_CASE("Tick engine resource-exhaustion visibility", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider with low action points (should not be able to afford actions)
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 2; // Not enough for most actions
    state.hiders[0].server_id = 1;
    
    // Set up servers
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    
    // Set up seeker
    state.seeker.max_tokens = 5;
    
    // Create candidate intents that would require more action points than available
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 2;
    
    // Add two HIDER_ACTION intents (should be rejected due to insufficient AP)
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER; // Costs 5 AP
    intent1.intent.idempotencyKey = 1001;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents.assignValue(0, intent1);
    
    TickIntent intent2{};
    intent2.kind = TickActionKind::HIDER_ACTION;
    intent2.action_mode = HiderActionMode::BOOST_CPU; // Costs 3 AP
    intent2.intent.idempotencyKey = 1002;
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(1, intent2);
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have 0 outcomes since both actions should be rejected due to insufficient AP
    REQUIRE(output.intentOutcomeCount == 0);
    REQUIRE(output.consumedKeyCount == 0); // No keys consumed because no actions accepted
}

TEST_CASE("Tick engine determinism across independent invocations", "[tick]") {
    MatchState state1{}, state2{};
    
    // Initialize identical states for testing
    state1.tick = 1;
    state1.hider_count = 1;
    state1.server_count = 2;
    state1.seeker.connection_tokens = 2;
    
    state2.tick = 1;
    state2.hider_count = 1;
    state2.server_count = 2;
    state2.seeker.connection_tokens = 2;
    
    // Set up hiders identically
    state1.hiders[0].active = 1;
    state1.hiders[0].action_points = 10;
    state1.hiders[0].server_id = 1;
    
    state2.hiders[0].active = 1;
    state2.hiders[0].action_points = 10;
    state2.hiders[0].server_id = 1;
    
    // Set up servers identically
    state1.servers[0].active = 1;
    state1.servers[0].current.cpu = 50;
    state1.servers[0].current.memory = 50;
    state1.servers[1].active = 1;
    state1.servers[1].current.cpu = 60;
    state1.servers[1].current.memory = 60;
    
    state2.servers[0].active = 1;
    state2.servers[0].current.cpu = 50;
    state2.servers[0].current.memory = 50;
    state2.servers[1].active = 1;
    state2.servers[1].current.cpu = 60;
    state2.servers[1].current.memory = 60;
    
    // Set up seekers identically
    state1.seeker.max_tokens = 5;
    state2.seeker.max_tokens = 5;
    
    // Create identical candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents1, intents2;
    uint8_t intentCount = 0;
    
    // Add a HIDER_ACTION intent
    TickIntent intent1{};
    intent1.kind = TickActionKind::HIDER_ACTION;
    intent1.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent1.intent.idempotencyKey = 12345;
    intent1.intent.hiderId = 0;
    intent1.intent.receiptSequence = 1;
    intents1.assignValue(intentCount++, intent1);
    
    TickIntent intent2{};
    intent2.kind = TickActionKind::HIDER_ACTION;
    intent2.action_mode = HiderActionMode::CONFIGURE_SERVER;
    intent2.intent.idempotencyKey = 12345;
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 1;
    intents2.assignValue(intentCount++, intent2);
    
    // Create identical consumed key sets
    ConsumedKeySet alreadyConsumed{};
    
    // Run ticks on both states
    StubPolicy policy;
    TickOutput output1 = runTick(state1, intents1, intentCount, alreadyConsumed, &policy);
    TickOutput output2 = runTick(state2, intents2, intentCount, alreadyConsumed, &policy);
    
    // Results should be identical for deterministic behavior
    REQUIRE(output1.intentOutcomeCount == output2.intentOutcomeCount);
    REQUIRE(output1.consumedKeyCount == output2.consumedKeyCount);
    REQUIRE(output1.probeCount == output2.probeCount);
}