#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "tick.hpp"
#include "intent_ordering.hpp"
#include "action_rules.hpp"
#include "loa_rules.hpp"
#include "state.hpp"

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

    struct ThreeCandidatePolicy : public iTargetRankingPolicy {
        void rankTargets(const MatchState&, Array<ProbeCandidate, MAX_SERVERS>& outCandidates) const override {
            outCandidates.assignValue(0, ProbeCandidate{0, 300});
            outCandidates.assignValue(1, ProbeCandidate{1, 200});
            outCandidates.assignValue(2, ProbeCandidate{2, 100});
        }
    };

    // Returns the recorded outcome for the given idempotency key; fails the
    // test if no outcome was recorded for it.
    RuleOutcome outcomeForKey(const TickOutput& output, IdempotencyKeyT key) {
        for (int i = 0; i < output.intentOutcomeCount; ++i) {
            IntentOutcomeRecord defaultRecord{};
            const IntentOutcomeRecord& record = output.intentOutcomes.getOrDefault(i, defaultRecord);
            if (record.idempotencyKey == key) {
                return record.outcome;
            }
        }
        FAIL("no outcome recorded for idempotency key " << key);
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    bool sameFrame(const MetricFrame& a, const MetricFrame& b) {
        return std::memcmp(&a, &b, sizeof(MetricFrame)) == 0;
    }

    // Builds a fully zeroed (padding included) state so two copies built the
    // same way are byte-identical before and after runTick.
    void buildDeterminismState(MatchState& state) {
        std::memset(&state, 0, sizeof(MatchState));
        state.tick = 7;
        state.rng_state = 0x1234ABCDull;
        state.hider_count = 2;
        state.server_count = 3;
        state.seeker.connection_tokens = 2;
        state.seeker.max_tokens = 5;

        state.hiders[0].active = 1;
        state.hiders[0].action_points = 10;
        state.hiders[0].server_id = 1;
        state.hiders[1].active = 1;
        state.hiders[1].action_points = 1;
        state.hiders[1].server_id = 2;

        for (int s = 0; s < 3; ++s) {
            state.servers[s].active = 1;
            state.servers[s].current.cpu = static_cast<uint8_t>(40 + s * 10);
            state.servers[s].current.memory = static_cast<uint8_t>(30 + s * 5);
            state.servers[s].trace_strength = 60;
            state.servers[s].trace_age = static_cast<uint8_t>(s);
        }
    }
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
    
    // CONFIGURE_SERVER is affordable (10 AP >= 5); ADD_SERVER is dispatched to
    // applyAddServer and rejected because the hider has no LOA.
    REQUIRE(output.intentOutcomeCount == 2);
    REQUIRE(output.consumedKeyCount == 1);
    REQUIRE(outcomeForKey(output, 12345) == RuleOutcome::ACCEPTED);
    REQUIRE(outcomeForKey(output, 12346) == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(state.server_count == 2);
    REQUIRE(state.servers[2].active == 0);
    REQUIRE(output.probeCount == 0); // No probes since policy returns no candidates
}

TEST_CASE("Tick engine ADD_SERVER dispatch and activation", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 2;
    state.seeker.connection_tokens = 2;
    
    // Set up hider with enough LOA to afford ADD_SERVER (costs 1 LOA)
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 10;
    state.hiders[0].server_id = 1;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 2;
    
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
    
    // Add an ADD_SERVER intent
    TickIntent intent{};
    intent.kind = TickActionKind::ADD_SERVER;
    intent.intent.idempotencyKey = 12346;
    intent.intent.hiderId = 0;
    intent.intent.receiptSequence = 1;
    intents.assignValue(0, intent);
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have one outcome (the ADD_SERVER action)
    REQUIRE(output.intentOutcomeCount == 1);
    REQUIRE(output.consumedKeyCount == 1);
    
    // The outcome should be ACCEPTED
    REQUIRE(outcomeForKey(output, 12346) == RuleOutcome::ACCEPTED);

    // applyAddServer claimed the lowest inactive slot (2) for the hider
    REQUIRE(state.servers[2].active == 1);
    REQUIRE(state.servers[2].owner_hider_id == 0);
    REQUIRE(state.servers[3].active == 0);
    REQUIRE(state.server_count == 3);
    // Exactly 1 LOA was spent; AP is untouched
    REQUIRE(state.hiders[0].resources[LOA_RESOURCE_INDEX] == 1);
    REQUIRE(state.hiders[0].action_points == 10);
}

TEST_CASE("Tick engine REMOVE_SERVER dispatch and slot freeing", "[tick]") {
    MatchState state{};
    
    // Initialize a basic state for testing
    state.tick = 1;
    state.hider_count = 1;
    state.server_count = 3;
    state.seeker.connection_tokens = 2;
    
    // Set up hider with enough LOA to afford REMOVE_SERVER (costs 1 LOA);
    // the hider occupies the slot being removed, so it must be relocated.
    state.hiders[0].active = 1;
    state.hiders[0].action_points = 5;
    state.hiders[0].server_id = 1;
    state.hiders[0].resources[LOA_RESOURCE_INDEX] = 1;
    
    // Set up servers - all active
    state.servers[0].active = 1;
    state.servers[0].current.cpu = 50;
    state.servers[0].current.memory = 50;
    state.servers[1].active = 1;
    state.servers[1].current.cpu = 60;
    state.servers[1].current.memory = 60;
    state.servers[2].active = 1;
    state.servers[2].current.cpu = 70;
    state.servers[2].current.memory = 70;
    
    // Set up seeker
    state.seeker.max_tokens = 5;
    
    // Create candidate intents
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    uint8_t intentCount = 1;
    
    // Add a REMOVE_SERVER intent - target an active slot (server 1)
    TickIntent intent{};
    intent.kind = TickActionKind::REMOVE_SERVER;
    intent.target_server_id = 1; // Remove server 1 which is active
    intent.intent.idempotencyKey = 12347;
    intent.intent.hiderId = 0;
    intent.intent.receiptSequence = 1;
    intents.assignValue(0, intent);
    
    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};
    
    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    
    // Should have one outcome (the REMOVE_SERVER action)
    REQUIRE(output.intentOutcomeCount == 1);
    REQUIRE(output.consumedKeyCount == 1);
    
    // The outcome should be ACCEPTED
    REQUIRE(outcomeForKey(output, 12347) == RuleOutcome::ACCEPTED);

    // applyRemoveServer freed exactly the named slot (1) and reset it
    REQUIRE(state.servers[1].active == 0);
    REQUIRE(state.servers[1].owner_hider_id == 0xFF);
    REQUIRE(state.servers[1].current.cpu == 0);
    REQUIRE(state.servers[0].active == 1);
    REQUIRE(state.servers[2].active == 1);
    REQUIRE(state.server_count == 2);
    // The occupant was relocated to the lowest other active slot (0)
    REQUIRE(state.hiders[0].server_id == 0);
    // Exactly 1 LOA was spent; AP is untouched
    REQUIRE(state.hiders[0].resources[LOA_RESOURCE_INDEX] == 0);
    REQUIRE(state.hiders[0].action_points == 5);
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
    
    // Inactive server with trace_strength > 0 must be untouched
    state.servers[2].active = 0;
    state.servers[2].trace_strength = 10;
    state.servers[2].trace_age = 4;

    const MetricFrame before0 = state.servers[0].current;
    const MetricFrame before1 = state.servers[1].current;
    const uint8_t head0 = state.servers[0].history_head;
    const uint8_t head1 = state.servers[1].history_head;

    // Run tick
    StubPolicy policy;
    TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);
    REQUIRE(output.probeCount == 0);

    // trace_age (a ServerState field) increments iff trace_strength > 0
    REQUIRE(state.servers[0].trace_age == 6); // 5 + 1
    REQUIRE(state.servers[1].trace_age == 3); // unchanged
    REQUIRE(state.servers[2].trace_age == 4); // inactive: unchanged
    REQUIRE(state.servers[0].trace_strength == 10);
    REQUIRE(state.servers[1].trace_strength == 0);

    // The pushed frame is the unmodified current frame, and no MetricFrame
    // field (errors included) was altered
    REQUIRE(state.servers[0].history_head == static_cast<uint8_t>(head0 + 1));
    REQUIRE(state.servers[1].history_head == static_cast<uint8_t>(head1 + 1));
    REQUIRE(sameFrame(state.servers[0].current, before0));
    REQUIRE(sameFrame(state.servers[1].current, before1));
    REQUIRE(sameFrame(state.servers[0].history[head0 % HISTORY_TICKS], before0));
    REQUIRE(sameFrame(state.servers[1].history[head1 % HISTORY_TICKS], before1));
    REQUIRE(state.servers[0].current.errors == 0);
    REQUIRE(state.servers[0].history[head0 % HISTORY_TICKS].errors == 0);
    REQUIRE(state.servers[2].history_head == 0);
}

TEST_CASE("Tick engine metrics placeholder leaves non-zero errors unchanged", "[tick]") {
    MatchState state{};
    state.servers[0].active = 1;
    state.servers[0].current.errors = 7;
    state.servers[0].trace_strength = 0;
    state.servers[0].trace_age = 2;
    state.servers[1].active = 1;
    state.servers[1].current.errors = 9;
    state.servers[1].trace_strength = 1;
    state.servers[1].trace_age = 2;

    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    ConsumedKeySet alreadyConsumed{};
    runTick(state, intents, 0, alreadyConsumed, nullptr);

    REQUIRE(state.servers[0].current.errors == 7);
    REQUIRE(state.servers[0].history[0].errors == 7);
    REQUIRE(state.servers[0].trace_age == 2);
    REQUIRE(state.servers[1].current.errors == 9);
    REQUIRE(state.servers[1].history[0].errors == 9);
    REQUIRE(state.servers[1].trace_age == 3);
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
    
    // trace_strength 0 everywhere: every probe misses, so no capture rewards
    // can refill the token budget mid-loop.

    SECTION("3 candidates, 2 tokens: only the first 2 in policy order are probed") {
        ThreeCandidatePolicy policy;
        TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);

        REQUIRE(output.probeCount == 2);
        REQUIRE(state.seeker.connection_tokens == 0);
        ProbeResult none{};
        REQUIRE(output.probeResults.getOrDefault(0, none).targetServerId == 0);
        REQUIRE(output.probeResults.getOrDefault(1, none).targetServerId == 1);
        REQUIRE(state.seeker.last_probe_tick[0] == 1);
        REQUIRE(state.seeker.last_probe_tick[1] == 1);
        REQUIRE(state.seeker.last_probe_tick[2] == 0); // truncated, never probed
    }

    SECTION("3 candidates, 1 token: exactly one probe and the budget never wraps") {
        state.seeker.connection_tokens = 1;
        ThreeCandidatePolicy policy;
        TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);

        REQUIRE(output.probeCount == 1);
        REQUIRE(state.seeker.connection_tokens == 0);
    }

    SECTION("3 candidates, 5 tokens: all 3 probed, one token each") {
        state.seeker.connection_tokens = 5;
        ThreeCandidatePolicy policy;
        TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);

        REQUIRE(output.probeCount == 3);
        REQUIRE(state.seeker.connection_tokens == 2);
    }
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
    
    StubPolicy policy;

    // First call: the intent is applied and its key reported as consumed
    ConsumedKeySet firstConsumed{};
    TickOutput first = runTick(state, intents, intentCount, firstConsumed, &policy);
    REQUIRE(first.intentOutcomeCount == 1);
    REQUIRE(first.consumedKeyCount == 1);
    REQUIRE(first.consumedKeys.getOrDefault(0, 0) == 12345);
    REQUIRE(state.hiders[0].action_points == 5); // CONFIGURE_SERVER costs 5

    // Carry the consumed keys from the prior call into the retry
    ConsumedKeySet alreadyConsumed{};
    for (int i = 0; i < first.consumedKeyCount; ++i) {
        alreadyConsumed.keys.assignValue(alreadyConsumed.count, first.consumedKeys.getOrDefault(i, 0));
        alreadyConsumed.count++;
    }

    // Retry of the same intent alongside a fresh one
    TickIntent fresh{};
    fresh.kind = TickActionKind::HIDER_ACTION;
    fresh.action_mode = HiderActionMode::BOOST_CPU;
    fresh.intent.idempotencyKey = 777;
    fresh.intent.hiderId = 0;
    fresh.intent.receiptSequence = 2;
    intents.assignValue(1, fresh);

    TickOutput retry = runTick(state, intents, 2, alreadyConsumed, &policy);

    // The retried key is filtered out; only the fresh intent is applied
    REQUIRE(retry.intentOutcomeCount == 1);
    IntentOutcomeRecord noOutcome{};
    REQUIRE(retry.intentOutcomes.getOrDefault(0, noOutcome).idempotencyKey == 777);
    REQUIRE(retry.consumedKeyCount == 1);
    REQUIRE(retry.consumedKeys.getOrDefault(0, 0) == 777);
    REQUIRE(state.hiders[0].action_points == 3); // only BOOST_CPU (2) charged
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
    state.hiders[0].action_points = 1; // Not enough for either action below
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
    intent2.action_mode = HiderActionMode::BOOST_CPU; // Costs 2 AP
    intent2.intent.idempotencyKey = 1002;
    intent2.intent.hiderId = 0;
    intent2.intent.receiptSequence = 2;
    intents.assignValue(1, intent2);

    // Create consumed key set
    ConsumedKeySet alreadyConsumed{};

    SECTION("insufficient action points are visible as rejected outcomes") {
        StubPolicy policy;
        const uint8_t cpuBefore = state.servers[1].current.cpu;
        TickOutput output = runTick(state, intents, intentCount, alreadyConsumed, &policy);

        // Both rejections are reported, in order, with the exhaustion reason
        REQUIRE(output.intentOutcomeCount == 2);
        IntentOutcomeRecord none{};
        REQUIRE(output.intentOutcomes.getOrDefault(0, none).idempotencyKey == 1001);
        REQUIRE(output.intentOutcomes.getOrDefault(0, none).outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
        REQUIRE(output.intentOutcomes.getOrDefault(1, none).idempotencyKey == 1002);
        REQUIRE(output.intentOutcomes.getOrDefault(1, none).outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
        REQUIRE(output.consumedKeyCount == 0); // Rejected keys are not consumed
        REQUIRE(state.hiders[0].action_points == 1); // Nothing charged
        REQUIRE(state.servers[1].current.cpu == cpuBefore); // No effect applied
    }

    SECTION("an exhausted seeker token budget yields no probes") {
        state.seeker.connection_tokens = 0;
        const uint64_t rngBefore = state.rng_state;
        MockPolicy policy;
        TickOutput output = runTick(state, intents, 0, alreadyConsumed, &policy);

        REQUIRE(output.probeCount == 0);
        REQUIRE(state.seeker.connection_tokens == 0); // never wraps below zero
        REQUIRE(state.rng_state == rngBefore); // no draw was made
        REQUIRE(state.seeker.last_probe_tick[1] == 0);
        REQUIRE(state.seeker.last_probe_tick[2] == 0);
    }
}

TEST_CASE("Tick engine determinism across independent invocations", "[tick]") {
    MatchState state1;
    MatchState state2;
    buildDeterminismState(state1);
    buildDeterminismState(state2);
    REQUIRE(std::memcmp(&state1, &state2, sizeof(MatchState)) == 0);

    // Identical candidate intents (accepted, rejected and a duplicate key),
    // deliberately supplied out of receipt order
    Array<TickIntent, MAX_INTENTS_PER_TICK> intents;
    const uint8_t intentCount = 4;

    TickIntent rotate{};
    rotate.kind = TickActionKind::HIDER_ACTION;
    rotate.action_mode = HiderActionMode::ROTATE_ROUTE;
    rotate.intent.idempotencyKey = 3003;
    rotate.intent.hiderId = 0;
    rotate.intent.receiptSequence = 3;
    intents.assignValue(0, rotate);

    TickIntent boost{};
    boost.kind = TickActionKind::HIDER_ACTION;
    boost.action_mode = HiderActionMode::BOOST_CPU;
    boost.intent.idempotencyKey = 1001;
    boost.intent.hiderId = 0;
    boost.intent.receiptSequence = 1;
    intents.assignValue(1, boost);

    TickIntent unaffordable{};
    unaffordable.kind = TickActionKind::HIDER_ACTION;
    unaffordable.action_mode = HiderActionMode::CONFIGURE_SERVER;
    unaffordable.intent.idempotencyKey = 2002;
    unaffordable.intent.hiderId = 1;
    unaffordable.intent.receiptSequence = 2;
    intents.assignValue(2, unaffordable);

    TickIntent duplicate = boost;
    duplicate.intent.receiptSequence = 4;
    intents.assignValue(3, duplicate);

    ConsumedKeySet alreadyConsumed{};
    MockPolicy policy;

    // Two independent invocations on identical inputs
    TickOutput output1 = runTick(state1, intents, intentCount, alreadyConsumed, &policy);
    TickOutput output2 = runTick(state2, intents, intentCount, alreadyConsumed, &policy);

    // The tick actually exercised actions, metrics and probes
    REQUIRE(output1.intentOutcomeCount == 3);
    REQUIRE(output1.probeCount == 2);

    // MatchState is byte-identical
    REQUIRE(std::memcmp(&state1, &state2, sizeof(MatchState)) == 0);

    // TickOutput is identical in every populated entry
    REQUIRE(output1.consumedKeyCount == output2.consumedKeyCount);
    for (int i = 0; i < output1.consumedKeyCount; ++i) {
        REQUIRE(output1.consumedKeys.getOrDefault(i, 0) == output2.consumedKeys.getOrDefault(i, 0));
    }

    REQUIRE(output1.intentOutcomeCount == output2.intentOutcomeCount);
    for (int i = 0; i < output1.intentOutcomeCount; ++i) {
        IntentOutcomeRecord none{};
        const IntentOutcomeRecord& a = output1.intentOutcomes.getOrDefault(i, none);
        const IntentOutcomeRecord& b = output2.intentOutcomes.getOrDefault(i, none);
        REQUIRE(a.idempotencyKey == b.idempotencyKey);
        REQUIRE(a.outcome == b.outcome);
    }

    REQUIRE(output1.probeCount == output2.probeCount);
    for (int i = 0; i < output1.probeCount; ++i) {
        ProbeResult noResult{};
        const ProbeResult& ra = output1.probeResults.getOrDefault(i, noResult);
        const ProbeResult& rb = output2.probeResults.getOrDefault(i, noResult);
        REQUIRE(ra.hit == rb.hit);
        REQUIRE(ra.targetServerId == rb.targetServerId);
        REQUIRE(ra.rngStateBefore == rb.rngStateBefore);
        REQUIRE(ra.rngCounter == rb.rngCounter);
        REQUIRE(ra.randomValue == rb.randomValue);

        ProbeConsequences noConsequences{};
        const ProbeConsequences& ca = output1.probeConsequences.getOrDefault(i, noConsequences);
        const ProbeConsequences& cb = output2.probeConsequences.getOrDefault(i, noConsequences);
        REQUIRE(std::memcmp(&ca, &cb, sizeof(ProbeConsequences)) == 0);
    }
}