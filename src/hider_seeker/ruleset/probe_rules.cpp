#include "probe_rules.hpp"
#include "rng.hpp"
#include <cstdint>

namespace hider_seeker {

// Placeholder constants for probe behavior - TODO: replace with final values
// These are placeholders as the design doc only gives hedged examples
constexpr uint8_t PLACEHOLDER_TOKEN_COST_PER_PROBE = 1;
// PLACEHOLDER_PROBE_HIT_THRESHOLD is now removed as it was unused

RuleOutcome applyProbe(MatchState& match, uint8_t serverId, ProbeResult& outResult) {
    if (serverId >= MAX_SERVERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }

    // Deduct the backend-defined per-probe connection-token cost first
    if (match.seeker.connection_tokens < PLACEHOLDER_TOKEN_COST_PER_PROBE) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }

    match.seeker.connection_tokens -= PLACEHOLDER_TOKEN_COST_PER_PROBE;

    // Compute hit/miss result as a deterministic function of MatchState plus the
    // server-held seeded RNG (rng.hpp). The draw consumes the match's RNG stream
    // (match.rng_state is advanced), so the sequence of probe results is fully
    // reproducible from the initial seed and the ordered action log.
    RngState rng;
    rng.state = match.rng_state;
    rng.counter = match.tick;
    uint32_t randomValue = nextRandomValue(rng);
    match.rng_state = rng.state;

    // Use the random value to determine hit/miss result
    // Simple approach: use lower bits of random value as probability
    uint8_t traceStrength = match.servers[serverId].trace_strength;
    
    // Convert random value to a percentage (0-100) and compare with trace strength
    // This makes the hit probability depend on both the trace strength and RNG
    uint8_t probability = static_cast<uint8_t>(randomValue % 100);
    outResult.targetServerId = serverId;
    outResult.hit = (probability < traceStrength);  // Higher trace strength = higher chance of hit
    
    return RuleOutcome::ACCEPTED;
}

void resolveProbeConsequences(MatchState& match, const ProbeResult& result) {
    // PROBE-001: Implement deterministic capture/escape/goal-qualification/reward logic
    
    if (result.hit) {
        // Handle capture logic
        ServerState& server = match.servers[result.targetServerId];
        
        // If the server is currently unowned, capture it
        if (server.owner_hider_id == 0xFF) {
            // Capture the server - make it owned by the seeker (seeker is hider 0)
            // In a real implementation, we might want to track who captured what
            server.owner_hider_id = 0; // Assuming seeker is hider 0 for capture purposes
            
            // Update server's public flags to indicate it was captured
            server.public_flags |= 0x01; // Capture flag
            
            // Update seeker's belief about this server (as if they now know it's captured)
            match.seeker.belief[result.targetServerId] = 1;
            
            // Update last probe tick for this server
            match.seeker.last_probe_tick[result.targetServerId] = match.tick;
        } else {
            // Server was already owned - handle escape or other logic
            // For now, just update belief and last probe tick
            match.seeker.belief[result.targetServerId] = 1;
            match.seeker.last_probe_tick[result.targetServerId] = match.tick;
            
            // If the server is owned by a different hider, this might trigger goal qualification
            // or other reward logic - but for now we'll just update state
        }
        
        // Apply goal qualification/reward logic based on capture/escape
        // This would be more complex in a real implementation
        for (uint8_t g = 0; g < MAX_GOALS; ++g) {
            if (!match.goals[g].active) continue;
            
            // Check if this goal is related to capturing servers
            for (uint8_t r = 0; r < match.goals[g].requirement_count; ++r) {
                Requirement& req = match.goals[g].requirements[r];
                
                // If this requirement is about server capture or ownership
                if (req.type == REQ_CONFIGURED_SERVER && req.subject_id == result.targetServerId) {
                    // This goal requirement might be satisfied by the capture
                    // In a real implementation, we'd check if the requirement is met
                    // and potentially update goal status
                }
            }
        }
    } else {
        // Miss - just update seeker's belief and last probe tick
        match.seeker.belief[result.targetServerId] = 0;
        match.seeker.last_probe_tick[result.targetServerId] = match.tick;
    }
}

} // namespace hider_seeker