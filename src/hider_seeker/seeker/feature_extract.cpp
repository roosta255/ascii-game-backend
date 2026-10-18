#include "feature_extract.hpp"
#include <algorithm>
#include <cstdint>

namespace hider_seeker {

// Convert a value to Q8 format (256 = 1.0) using integer division truncating toward zero
int16_t toQ8(int v) {
    return static_cast<int16_t>(v * 256 / 100);
}

// Get frame k ticks ago from server history
MetricFrame frameAgo(const ServerState& server, int k) {
    return server.history[(server.history_head + HISTORY_TICKS - 1 - k) % HISTORY_TICKS];
}

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs{};
    
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const ServerState& server = state.servers[s];
        
        // Set the active feature (index 0)
        obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
            row.set(0, server.active ? int16_t{256} : int16_t{0});
        });
        
        // Skip processing if server is inactive
        if (!server.active) {
            continue;
        }
        
        // Set seeker connected feature (index 1)
        const bool connected = (state.seeker_connection_mask & (1U << s)) != 0;
        obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
            row.set(1, connected ? int16_t{256} : int16_t{0});
        });
        
        if (connected) {
            // Indices 2-8: current metrics
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(2, toQ8(server.current.cpu));
                row.set(3, toQ8(server.current.memory));
                row.set(4, toQ8(server.current.disk));
                row.set(5, toQ8(server.current.network));
                row.set(6, toQ8(server.current.heat));
                row.set(7, toQ8(server.current.errors));
                row.set(8, toQ8(server.current.users));
            });
            
            // Indices 9-36: changes over k = 1, 2, 4, 8 ticks for each metric
            // k=1 (indices 9-15)
            const int k1_start = 9;
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(k1_start + 0, toQ8(server.current.cpu - frameAgo(server, 1).cpu));
                row.set(k1_start + 1, toQ8(server.current.memory - frameAgo(server, 1).memory));
                row.set(k1_start + 2, toQ8(server.current.disk - frameAgo(server, 1).disk));
                row.set(k1_start + 3, toQ8(server.current.network - frameAgo(server, 1).network));
                row.set(k1_start + 4, toQ8(server.current.heat - frameAgo(server, 1).heat));
                row.set(k1_start + 5, toQ8(server.current.errors - frameAgo(server, 1).errors));
                row.set(k1_start + 6, toQ8(server.current.users - frameAgo(server, 1).users));
            });
            
            // k=2 (indices 16-22)
            const int k2_start = 16;
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(k2_start + 0, toQ8(server.current.cpu - frameAgo(server, 2).cpu));
                row.set(k2_start + 1, toQ8(server.current.memory - frameAgo(server, 2).memory));
                row.set(k2_start + 2, toQ8(server.current.disk - frameAgo(server, 2).disk));
                row.set(k2_start + 3, toQ8(server.current.network - frameAgo(server, 2).network));
                row.set(k2_start + 4, toQ8(server.current.heat - frameAgo(server, 2).heat));
                row.set(k2_start + 5, toQ8(server.current.errors - frameAgo(server, 2).errors));
                row.set(k2_start + 6, toQ8(server.current.users - frameAgo(server, 2).users));
            });
            
            // k=4 (indices 23-29)
            const int k4_start = 23;
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(k4_start + 0, toQ8(server.current.cpu - frameAgo(server, 4).cpu));
                row.set(k4_start + 1, toQ8(server.current.memory - frameAgo(server, 4).memory));
                row.set(k4_start + 2, toQ8(server.current.disk - frameAgo(server, 4).disk));
                row.set(k4_start + 3, toQ8(server.current.network - frameAgo(server, 4).network));
                row.set(k4_start + 4, toQ8(server.current.heat - frameAgo(server, 4).heat));
                row.set(k4_start + 5, toQ8(server.current.errors - frameAgo(server, 4).errors));
                row.set(k4_start + 6, toQ8(server.current.users - frameAgo(server, 4).users));
            });
            
            // k=8 (indices 30-36)
            const int k8_start = 30;
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(k8_start + 0, toQ8(server.current.cpu - frameAgo(server, 8).cpu));
                row.set(k8_start + 1, toQ8(server.current.memory - frameAgo(server, 8).memory));
                row.set(k8_start + 2, toQ8(server.current.disk - frameAgo(server, 8).disk));
                row.set(k8_start + 3, toQ8(server.current.network - frameAgo(server, 8).network));
                row.set(k8_start + 4, toQ8(server.current.heat - frameAgo(server, 8).heat));
                row.set(k8_start + 5, toQ8(server.current.errors - frameAgo(server, 8).errors));
                row.set(k8_start + 6, toQ8(server.current.users - frameAgo(server, 8).users));
            });
            
            // Index 37: ticks since last wager
            const int tick_diff = (state.tick - state.seeker.last_probe_tick[s]) & 0xFF;
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(37, static_cast<int16_t>(std::min<int>(tick_diff, 32) * 8));
            });
            
            // Index 38: hider connections destroyed by last wager
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                row.set(38, static_cast<int16_t>(std::min<int>(state.seeker.belief[s], 8) * 32));
            });
        } else {
            // If not connected, set all features 2-36 to 0
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                for (int i = 2; i <= 36; ++i) {
                    row.set(i, int16_t{0});
                }
            });
        }
    }
    
    // Compute global features
    obs.global.access(0, [&](int16_t& value) {
        value = static_cast<int16_t>(std::min<int>(state.seeker.connection_tokens, 32U) * 8);
    });
    obs.global.access(1, [&](int16_t& value) {
        value = static_cast<int16_t>(__builtin_popcount(state.seeker_connection_mask) * 16);
    });
    obs.global.access(2, [&](int16_t& value) {
        value = static_cast<int16_t>(state.server_count * 16);
    });
    obs.global.access(3, [&](int16_t& value) {
        value = static_cast<int16_t>(std::min<int>(state.tick, 256U));
    });
    
    return obs;
}

} // namespace hider_seeker