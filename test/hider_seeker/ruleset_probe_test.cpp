#include <catch2/catch_test_macros.hpp>
#include "probe_rules.hpp"
#include "state.hpp"

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

TEST_CASE("resolveProbeConsequences handles hit correctly", "[probe]") {
    MatchState match = {};
    
    // Set up a server with some trace strength
    match.servers[0].trace_strength = 60; // High enough to be hit
    
    ProbeResult result;
    result.hit = true;
    result.targetServerId = 0;
    
    // Call resolveProbeConsequences
    resolveProbeConsequences(match, result);
    
    // Should have captured the server (if it was unowned)
    REQUIRE(match.servers[0].owner_hider_id == 0); // Assuming seeker is hider 0
    REQUIRE(match.servers[0].public_flags & 0x01); // Capture flag set
    
    // Seeker belief should be updated
    REQUIRE(match.seeker.belief[0] == 1);
    
    // Last probe tick should be updated
    REQUIRE(match.seeker.last_probe_tick[0] == match.tick);
}

TEST_CASE("resolveProbeConsequences handles miss correctly", "[probe]") {
    MatchState match = {};
    
    ProbeResult result;
    result.hit = false;
    result.targetServerId = 0;
    
    // Call resolveProbeConsequences
    resolveProbeConsequences(match, result);
    
    // Seeker belief should be updated to 0 (miss)
    REQUIRE(match.seeker.belief[0] == 0);
    
    // Last probe tick should be updated
    REQUIRE(match.seeker.last_probe_tick[0] == match.tick);
}