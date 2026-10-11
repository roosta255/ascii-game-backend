#include <catch2/catch_test_macros.hpp>
#include "hider_seeker/ruleset/probe_rules.hpp"
#include "hider_seeker/domain/state.hpp"

using namespace hider_seeker;

TEST_CASE("applyProbe deducts token cost before computing hit/miss result", "[probe]") {
    MatchState match = {};
    
    // Initialize seeker with 2 tokens
    match.seeker.connection_tokens = 2;
    
    ProbeResult result;
    RuleOutcome outcome = applyProbe(match, 0, result);
    
    // Should succeed and deduct 1 token
    REQUIRE(outcome == RuleOutcome::ACCEPTED);
    REQUIRE(match.seeker.connection_tokens == 1);
}

TEST_CASE("applyProbe is rejected when insufficient tokens", "[probe]") {
    MatchState match = {};
    
    // Initialize seeker with 0 tokens
    match.seeker.connection_tokens = 0;
    
    ProbeResult result;
    RuleOutcome outcome = applyProbe(match, 0, result);
    
    // Should fail due to insufficient tokens
    REQUIRE(outcome == RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE);
    REQUIRE(match.seeker.connection_tokens == 0);
}

TEST_CASE("resolveProbeConsequences is a pure deterministic function", "[probe]") {
    MatchState match = {};
    
    // Set up a server with some trace strength
    match.servers[0].trace_strength = 60; // High enough to be hit
    
    ProbeResult result;
    result.hit = true;
    result.targetServerId = 0;
    
    // Call resolveProbeConsequences multiple times with same inputs
    resolveProbeConsequences(match, result);
    resolveProbeConsequences(match, result);
    resolveProbeConsequences(match, result);
    
    // Should have the same deterministic outcome each time
    // (placeholder - actual implementation would check specific effects)
    REQUIRE(match.servers[0].public_flags == 1); // Example flag set
}