#include "feature_extract.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace hider_seeker {

namespace {
    int16_t toQ8(int v) {
        return static_cast<int16_t>(v * 256 / 100);
    }

    MetricFrame frameAgo(const ServerState& server, int k) {
        // Handle negative indices properly
        int historyIndex = (server.history_head - 1 - k);
        if (historyIndex < 0) {
            historyIndex += HISTORY_TICKS;
        }
        return server.history[historyIndex];
    }
    
    // Simple popcount implementation for uint16_t
    inline int popcount(uint16_t x) {
        int count = 0;
        while (x) {
            count += x & 1;
            x >>= 1;
        }
        return count;
    }
} // anonymous namespace

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs = {};
    
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const ServerState& server = state.servers[s];
        
        // Determine if this server is "connected" from seeker's perspective
        bool connected = ((state.seeker_connection_mask >> s) & 1);
        
        // For inactive servers, all features should be 0
        if (!server.active) {
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
                    f.set(i, int16_t{0});
                }
            });
        } else {
            // Server is active - set features appropriately
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                // Feature index 0: active (256 if server is active, 0 otherwise)
                f.set(0, int16_t{256});
                
                // Feature index 1: seeker connected (256 if connected, 0 otherwise)
                f.set(1, connected ? int16_t{256} : int16_t{0});
            });
            
            if (connected) {
                // Server is active and seeker connected - compute all metrics features
                
                // Features indices 2-8: current metrics
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(2, toQ8(server.current.cpu));
                    f.set(3, toQ8(server.current.memory));
                    f.set(4, toQ8(server.current.disk));
                    f.set(5, toQ8(server.current.network));
                    f.set(6, toQ8(server.current.heat));
                    f.set(7, toQ8(server.current.errors));
                    f.set(8, toQ8(server.current.users));
                });
                
                // Features indices 9-36: change over k = 1, 2, 4, 8 ticks for each metric
                // k=1 (indices 9-15)
                const MetricFrame frame1 = frameAgo(server, 1);
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(9, toQ8(server.current.cpu - frame1.cpu));
                    f.set(10, toQ8(server.current.memory - frame1.memory));
                    f.set(11, toQ8(server.current.disk - frame1.disk));
                    f.set(12, toQ8(server.current.network - frame1.network));
                    f.set(13, toQ8(server.current.heat - frame1.heat));
                    f.set(14, toQ8(server.current.errors - frame1.errors));
                    f.set(15, toQ8(server.current.users - frame1.users));
                });
                
                // k=2 (indices 16-22)
                const MetricFrame frame2 = frameAgo(server, 2);
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(16, toQ8(server.current.cpu - frame2.cpu));
                    f.set(17, toQ8(server.current.memory - frame2.memory));
                    f.set(18, toQ8(server.current.disk - frame2.disk));
                    f.set(19, toQ8(server.current.network - frame2.network));
                    f.set(20, toQ8(server.current.heat - frame2.heat));
                    f.set(21, toQ8(server.current.errors - frame2.errors));
                    f.set(22, toQ8(server.current.users - frame2.users));
                });
                
                // k=4 (indices 23-29)
                const MetricFrame frame4 = frameAgo(server, 4);
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(23, toQ8(server.current.cpu - frame4.cpu));
                    f.set(24, toQ8(server.current.memory - frame4.memory));
                    f.set(25, toQ8(server.current.disk - frame4.disk));
                    f.set(26, toQ8(server.current.network - frame4.network));
                    f.set(27, toQ8(server.current.heat - frame4.heat));
                    f.set(28, toQ8(server.current.errors - frame4.errors));
                    f.set(29, toQ8(server.current.users - frame4.users));
                });
                
                // k=8 (indices 30-36)
                const MetricFrame frame8 = frameAgo(server, 8);
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(30, toQ8(server.current.cpu - frame8.cpu));
                    f.set(31, toQ8(server.current.memory - frame8.memory));
                    f.set(32, toQ8(server.current.disk - frame8.disk));
                    f.set(33, toQ8(server.current.network - frame8.network));
                    f.set(34, toQ8(server.current.heat - frame8.heat));
                    f.set(35, toQ8(server.current.errors - frame8.errors));
                    f.set(36, toQ8(server.current.users - frame8.users));
                });
                
                // Index 37: ticks since last wager
                const int tickDiff = (state.tick - state.seeker.last_probe_tick[s]) & 0xFF;
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(37, std::min(tickDiff, 32) * 8);
                });
                
                // Index 38: hider connections destroyed by the last wager
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    f.set(38, std::min(state.seeker.belief[s], uint8_t{8}) * 32);
                });
            } else {
                // Server is active but seeker not connected - set all metrics features to 0
                obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f) {
                    for (int i = 2; i < SEEKER_SERVER_FEATURES; ++i) {
                        f.set(i, int16_t{0});
                    }
                });
            }
        }
    }
    
    // Compute global features
    obs.global.access(0, [&](int16_t& value) { value = std::min(state.seeker.connection_tokens, uint8_t{32}) * 8; });  // seeker pool
    obs.global.access(1, [&](int16_t& value) { value = popcount(state.seeker_connection_mask) * 16; });                // open seeker connections
    obs.global.access(2, [&](int16_t& value) { 
        int count = 0;
        for (const auto& server : state.servers) {
            if (server.active) {
                count++;
            }
        }
        value = count * 16; 
    });  // active servers
    obs.global.access(3, [&](int16_t& value) { value = std::min(state.tick, uint32_t{256}); });                        // tick
    
    return obs;
}

} // namespace hider_seeker