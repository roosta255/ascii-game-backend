#include "feature_extract.hpp"
#include <algorithm>

namespace hider_seeker {

namespace {
    // Helper function to compute Q8 value: v * 256 / 100 using integer division
    int16_t q8(uint8_t v) {
        return static_cast<int16_t>(v * 256 / 100);
    }

    // Helper function to get frame k ticks ago (with wraparound)
    const MetricFrame& frameAgo(const ServerState& server, int k) {
        int index = (server.history_head - 1 - k) & 0xF;
        return server.history[index];
    }
    
    // Helper function for popcount since it's not defined in the codebase
    inline int popcount(uint16_t mask) {
        return __builtin_popcount(mask);
    }
}

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs{};
    
    // Process each server - using index s for bit operations (seeker_connection_mask)
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const ServerState& server = state.servers[s];
        
        // Per-server features
        obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            // Feature 0: active
            f.set(0, server.active ? 256 : 0);
            
            // Skip processing if server is inactive
            if (!server.active) {
                return;
            }
            
            // Feature 1: seeker connected
            bool connected = (state.seeker_connection_mask & (1 << s)) != 0;
            f.set(1, connected ? 256 : 0);
            
            // Features 2-8: current metrics (Q8)
            if (connected) {
                f.set(2, q8(server.current.cpu));
                f.set(3, q8(server.current.memory));
                f.set(4, q8(server.current.disk));
                f.set(5, q8(server.current.network));
                f.set(6, q8(server.current.heat));
                f.set(7, q8(server.current.errors));
                f.set(8, q8(server.current.users));
            } else {
                f.set(2, 0);
                f.set(3, 0);
                f.set(4, 0);
                f.set(5, 0);
                f.set(6, 0);
                f.set(7, 0);
                f.set(8, 0);
            }
            
            // Features 9-36: changes over k = 1, 2, 4, 8 ticks for each metric
            if (connected) {
                // For each of the 7 metrics, compute change for k=1,2,4,8
                for (int k : {1, 2, 4, 8}) {
                    int baseIndex = 9 + (k - 1) * 7; // k-major indexing
                    
                    const MetricFrame& past = frameAgo(server, k);
                    
                    // Compute change and then apply Q8 conversion
                    int16_t delta_cpu = static_cast<int16_t>(server.current.cpu - past.cpu);
                    int16_t delta_memory = static_cast<int16_t>(server.current.memory - past.memory);
                    int16_t delta_disk = static_cast<int16_t>(server.current.disk - past.disk);
                    int16_t delta_network = static_cast<int16_t>(server.current.network - past.network);
                    int16_t delta_heat = static_cast<int16_t>(server.current.heat - past.heat);
                    int16_t delta_errors = static_cast<int16_t>(server.current.errors - past.errors);
                    int16_t delta_users = static_cast<int16_t>(server.current.users - past.users);
                    
                    // Apply Q8 conversion (v * 256 / 100) to each change
                    f.set(baseIndex + 0, delta_cpu * 256 / 100);     // cpu Δk
                    f.set(baseIndex + 1, delta_memory * 256 / 100); // memory Δk
                    f.set(baseIndex + 2, delta_disk * 256 / 100);    // disk Δk
                    f.set(baseIndex + 3, delta_network * 256 / 100); // network Δk
                    f.set(baseIndex + 4, delta_heat * 256 / 100);    // heat Δk
                    f.set(baseIndex + 5, delta_errors * 256 / 100); // errors Δk
                    f.set(baseIndex + 6, delta_users * 256 / 100);  // users Δk
                }
            } else {
                // Set all change features to 0 if not connected
                for (int i = 9; i < 37; ++i) {
                    f.set(i, 0);
                }
            }
            
            // Feature 37: ticks since last wager
            int16_t ticksSinceWager = ((state.tick - state.seeker.last_probe_tick[s]) & 0xFF);
            ticksSinceWager = (ticksSinceWager < 32) ? ticksSinceWager : 32;
            f.set(37, ticksSinceWager * 8);
            
            // Feature 38: hider connections destroyed by last wager
            int16_t belief = (state.seeker.belief[s] < 8) ? state.seeker.belief[s] : 8;
            f.set(38, belief * 32);
        });
    }
    
    // Global features - compute them directly without using access() method
    // Feature 0: seeker pool
    int16_t tokens = (state.seeker.connection_tokens < 32) ? state.seeker.connection_tokens : 32;
    obs.global.set(0, tokens * 8);
    
    // Feature 1: open seeker connections
    int16_t connectionCount = popcount(state.seeker_connection_mask);
    obs.global.set(1, connectionCount * 16);
    
    // Feature 2: active servers - we need index s for counting
    int16_t activeCount = 0;
    for (int s = 0; s < MAX_SERVERS; ++s) {
        if (state.servers[s].active) {
            activeCount++;
        }
    }
    obs.global.set(2, activeCount * 16);
    
    // Feature 3: tick
    int16_t tick = (state.tick < 256) ? state.tick : 256;
    obs.global.set(3, tick);
    
    return obs;
}

} // namespace hider_seeker