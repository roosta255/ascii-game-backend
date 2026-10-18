#include "feature_extract.hpp"
#include <algorithm>
#include <cstdint>

namespace hider_seeker {

// Helper function to convert a value to Q8 format (256 = 1.0)
int16_t toQ8(int v) {
    return static_cast<int16_t>(v * 256 / 100);
}

// Helper function to compute frameAgo
MetricFrame frameAgo(const ServerState& server, int k) {
    int index = (server.history_head - 1 - k) % 16;
    if (index < 0) index += 16; // Handle negative modulo
    return server.history[index];
}

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs{};
    
    for (size_t s = 0; s < MAX_SERVERS; ++s) {
        // Access the row for this server using access method
        Array<int16_t, SEEKER_SERVER_FEATURES> f;
        obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& f_ref) {
            f = f_ref;
            
            // Index 0: active flag
            f.set(0, state.servers[s].active ? 256 : 0);
            
            // Index 1: seeker connected flag
            f.set(1, (state.seeker_connection_mask & (1 << s)) ? 256 : 0);
            
            bool connected = (f.getOrDefault(1, int16_t{0}) != 0);
            
            if (connected) {
                // Q8 conversion: v * 256 / 100 (integer division truncating toward zero)
                f.set(2, toQ8(state.servers[s].current.cpu));
                f.set(3, toQ8(state.servers[s].current.memory));
                f.set(4, toQ8(state.servers[s].current.disk));
                f.set(5, toQ8(state.servers[s].current.network));
                f.set(6, toQ8(state.servers[s].current.heat));
                f.set(7, toQ8(state.servers[s].current.errors));
                f.set(8, toQ8(state.servers[s].current.users));
                
                // Indices 9-36: signed change Q8 of current.m - frameAgo(s,k).m
                // for each metric m (cpu,...,users) and each major k (1,2,4,8)
                // k-major: indices 9–15 are k=1, 16–22 are k=2, 23–29 are k=4, 30–36 are k=8
                
                // For k = 1 (indices 9-15) - cpu
                int delta = state.servers[s].current.cpu - frameAgo(state.servers[s], 1).cpu;
                f.set(9, toQ8(delta));
                
                // For k = 1 (indices 9-15) - memory  
                delta = state.servers[s].current.memory - frameAgo(state.servers[s], 1).memory;
                f.set(10, toQ8(delta));
                
                // For k = 1 (indices 9-15) - disk
                delta = state.servers[s].current.disk - frameAgo(state.servers[s], 1).disk;
                f.set(11, toQ8(delta));
                
                // For k = 1 (indices 9-15) - network
                delta = state.servers[s].current.network - frameAgo(state.servers[s], 1).network;
                f.set(12, toQ8(delta));
                
                // For k = 1 (indices 9-15) - heat
                delta = state.servers[s].current.heat - frameAgo(state.servers[s], 1).heat;
                f.set(13, toQ8(delta));
                
                // For k = 1 (indices 9-15) - errors
                delta = state.servers[s].current.errors - frameAgo(state.servers[s], 1).errors;
                f.set(14, toQ8(delta));
                
                // For k = 1 (indices 9-15) - users
                delta = state.servers[s].current.users - frameAgo(state.servers[s], 1).users;
                f.set(15, toQ8(delta));
                
                // For k = 2 (indices 16-22)
                delta = state.servers[s].current.cpu - frameAgo(state.servers[s], 2).cpu;
                f.set(16, toQ8(delta));
                
                delta = state.servers[s].current.memory - frameAgo(state.servers[s], 2).memory;
                f.set(17, toQ8(delta));
                
                delta = state.servers[s].current.disk - frameAgo(state.servers[s], 2).disk;
                f.set(18, toQ8(delta));
                
                delta = state.servers[s].current.network - frameAgo(state.servers[s], 2).network;
                f.set(19, toQ8(delta));
                
                delta = state.servers[s].current.heat - frameAgo(state.servers[s], 2).heat;
                f.set(20, toQ8(delta));
                
                delta = state.servers[s].current.errors - frameAgo(state.servers[s], 2).errors;
                f.set(21, toQ8(delta));
                
                delta = state.servers[s].current.users - frameAgo(state.servers[s], 2).users;
                f.set(22, toQ8(delta));
                
                // For k = 4 (indices 23-29)
                delta = state.servers[s].current.cpu - frameAgo(state.servers[s], 4).cpu;
                f.set(23, toQ8(delta));
                
                delta = state.servers[s].current.memory - frameAgo(state.servers[s], 4).memory;
                f.set(24, toQ8(delta));
                
                delta = state.servers[s].current.disk - frameAgo(state.servers[s], 4).disk;
                f.set(25, toQ8(delta));
                
                delta = state.servers[s].current.network - frameAgo(state.servers[s], 4).network;
                f.set(26, toQ8(delta));
                
                delta = state.servers[s].current.heat - frameAgo(state.servers[s], 4).heat;
                f.set(27, toQ8(delta));
                
                delta = state.servers[s].current.errors - frameAgo(state.servers[s], 4).errors;
                f.set(28, toQ8(delta));
                
                delta = state.servers[s].current.users - frameAgo(state.servers[s], 4).users;
                f.set(29, toQ8(delta));
                
                // For k = 8 (indices 30-36)
                delta = state.servers[s].current.cpu - frameAgo(state.servers[s], 8).cpu;
                f.set(30, toQ8(delta));
                
                delta = state.servers[s].current.memory - frameAgo(state.servers[s], 8).memory;
                f.set(31, toQ8(delta));
                
                delta = state.servers[s].current.disk - frameAgo(state.servers[s], 8).disk;
                f.set(32, toQ8(delta));
                
                delta = state.servers[s].current.network - frameAgo(state.servers[s], 8).network;
                f.set(33, toQ8(delta));
                
                delta = state.servers[s].current.heat - frameAgo(state.servers[s], 8).heat;
                f.set(34, toQ8(delta));
                
                delta = state.servers[s].current.errors - frameAgo(state.servers[s], 8).errors;
                f.set(35, toQ8(delta));
                
                delta = state.servers[s].current.users - frameAgo(state.servers[s], 8).users;
                f.set(36, toQ8(delta));
            } else {
                // All metric indices 2-36 are 0 when not connected.
                for (int i = 2; i <= 36; ++i) {
                    f.set(i, int16_t{0});
                }
            }
            
            // Index 37: ticks since last wager
            uint8_t tickDiff = (state.tick - state.seeker.last_probe_tick[s]) & 0xFF;
            f.set(37, std::min(tickDiff, uint8_t{32}) * 8);
            
            // Index 38: hider connections destroyed by the last wager
            f.set(38, std::min(state.seeker.belief[s], uint8_t{8}) * 32);
            
            // If server is inactive, set the entire row to 0.
            if (!state.servers[s].active) {
                for (size_t i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
                    f.set(i, int16_t{0});
                }
            }
            
            // Copy back the modified row
            f_ref = f;
        });
    }
    
    // Global features (indices 0-3):
    obs.global.set(0, std::min(state.seeker.connection_tokens, uint8_t{32}) * 8);
    obs.global.set(1, __builtin_popcount(state.seeker_connection_mask) * 16);
    // Count active servers
    int activeCount = 0;
    for (const auto& server : state.servers) {
        if (server.active) {
            activeCount++;
        }
    }
    obs.global.set(2, static_cast<uint8_t>(activeCount) * 16);
    obs.global.set(3, static_cast<uint8_t>(std::min(static_cast<int>(state.tick), 256)));
    
    return obs;
}

} // namespace hider_seeker