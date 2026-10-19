#include "feature_extract.hpp"
#include <algorithm>

namespace hider_seeker {

// Convert a value to Q8 (256 = 1.0) using integer division truncating toward zero
int16_t toQ8(int v) {
    return static_cast<int16_t>(v * 256 / 100);
}

// Get frame k ticks ago from server history
MetricFrame frameAgo(const ServerState& server, int k) {
    // history_head is the next slot to write, so newest frame is at (history_head - 1)
    // frameAgo(k) is history[(history_head - 1 - k) % HISTORY_TICKS]
    return server.history[(server.history_head + HISTORY_TICKS - 1 - k) % HISTORY_TICKS];
}

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs;
    
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const ServerState& server = state.servers[s];
        
        // Set all features to 0 initially (in case server is inactive)
        obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
            for (auto& feature : f) {
                feature = int16_t{0};
            }
            
            // Index 0: active
            if (server.active) {
                f.set(0, int16_t{256});
            } else {
                return; // Skip all other features for inactive servers
            }
            
            // Index 1: seeker connected
            if (state.seeker_connection_mask & (1 << s)) {
                f.set(1, int16_t{256});
            }
            
            // Indices 2-8: current metrics when connected
            if (f.getOrDefault(1, int16_t{0}) == 256) {
                f.set(2, toQ8(server.current.cpu));
                f.set(3, toQ8(server.current.memory));
                f.set(4, toQ8(server.current.disk));
                f.set(5, toQ8(server.current.network));
                f.set(6, toQ8(server.current.heat));
                f.set(7, toQ8(server.current.errors));
                f.set(8, toQ8(server.current.users));
            }
            
            // Indices 9-36: change over k=1,2,4,8 ticks for each metric when connected
            if (f.getOrDefault(1, int16_t{0}) == 256) {
                constexpr int K_VALUES[] = {1, 2, 4, 8};
                constexpr int BLOCK_STARTS[] = {9, 16, 23, 30}; // Start indices for each k-block
                const int METRIC_COUNT = 7; // cpu, memory, disk, network, heat, errors, users
                
                // For each k value (1, 2, 4, 8)
                for (int block = 0; block < 4; ++block) {
                    const int k = K_VALUES[block];
                    const int firstIndex = BLOCK_STARTS[block];
                    
                    // Get frame k ticks ago
                    MetricFrame frame = frameAgo(server, k);
                    
                    // Compute change for each metric and store in appropriate block
                    f.set(firstIndex + 0, toQ8(server.current.cpu - frame.cpu));     // cpu
                    f.set(firstIndex + 1, toQ8(server.current.memory - frame.memory)); // memory
                    f.set(firstIndex + 2, toQ8(server.current.disk - frame.disk));    // disk
                    f.set(firstIndex + 3, toQ8(server.current.network - frame.network)); // network
                    f.set(firstIndex + 4, toQ8(server.current.heat - frame.heat));    // heat
                    f.set(firstIndex + 5, toQ8(server.current.errors - frame.errors)); // errors
                    f.set(firstIndex + 6, toQ8(server.current.users - frame.users));  // users
                }
            }
            
            // Index 37: ticks since last wager (computed regardless of connection)
            int tickDiff = (state.tick - static_cast<uint32_t>(state.seeker.last_probe_tick[s])) & 0xFF;
            f.set(37, static_cast<int16_t>(std::min(static_cast<int>(tickDiff), 32) * 8));
            
            // Index 38: hider connections destroyed by last wager (computed regardless of connection)
            f.set(38, static_cast<int16_t>(std::min(state.seeker.belief[s], static_cast<uint8_t>(8)) * 32));
        });
    }
    
    // Compute global features
    obs.global.access(0, [&](int16_t& value) {
        value = static_cast<int16_t>(std::min(static_cast<int>(state.seeker.connection_tokens), 32) * 8);
    });
    obs.global.access(1, [&](int16_t& value) {
        // Count set bits in seeker_connection_mask
        int count = 0;
        uint16_t mask = state.seeker_connection_mask;
        while (mask) {
            count += mask & 1;
            mask >>= 1;
        }
        value = static_cast<int16_t>(count * 16);
    });
    obs.global.access(2, [&](int16_t& value) {
        // Count active servers
        int count = 0;
        for (const auto& server : state.servers) {
            if (server.active) {
                count++;
            }
        }
        value = static_cast<int16_t>(count * 16);
    });
    obs.global.access(3, [&](int16_t& value) {
        // Compute tick feature as per design: min(tick, 256)
        value = static_cast<int16_t>(std::min(static_cast<int>(state.tick), 256));
    });
    
    return obs;
}

} // namespace hider_seeker