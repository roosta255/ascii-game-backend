#include "feature_extract.hpp"
#include "../../data/Array.hpp"
#include <algorithm>

// Helper to convert a value to Q8 (256 = 1.0) using integer division truncating toward zero
int16_t toQ8(int v) {
    return static_cast<int16_t>(v * 256 / 100);
}

namespace hider_seeker {

SeekerObservation buildSeekerObservation(const MatchState& state) {
    SeekerObservation obs;
    
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        const auto& server = state.servers[s];
        
        // Set the active flag for this server
        const bool isActive = server.active != 0;
        
        if (!isActive) {
            // For inactive servers, all features are 0
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                for (auto& x : row) {
                    x = int16_t{0};
                }
            });
        } else {
            // Set the seeker connected flag for this server
            const bool isConnected = (state.seeker_connection_mask & (1 << s)) != 0;
            
            obs.server.access(s, [&](Array<int16_t, SEEKER_SERVER_FEATURES>& row) {
                // Index 0: active (256 if server is active, else 0)
                row.set(0, isActive ? int16_t{256} : int16_t{0});
                
                // Index 1: seeker connected (256 if seeker is connected to this server, else 0)
                row.set(1, isConnected ? int16_t{256} : int16_t{0});
                
                // Indices 2-8: Q8 values of current metrics when connected; otherwise 0
                if (isConnected) {
                    row.set(2, toQ8(server.current.cpu));
                    row.set(3, toQ8(server.current.memory));
                    row.set(4, toQ8(server.current.disk));
                    row.set(5, toQ8(server.current.network));
                    row.set(6, toQ8(server.current.heat));
                    row.set(7, toQ8(server.current.errors));
                    row.set(8, toQ8(server.current.users));
                } else {
                    for (int i = 2; i <= 8; ++i) {
                        row.set(i, int16_t{0});
                    }
                }
                
                // Indices 9-36: Q8 of signed change current.m - frameAgo(s, k).m for each metric m and each k-major
                if (isConnected) {
                    // Process k=1 (indices 9-15)
                    row.set(9, toQ8(server.current.cpu - server.history[(server.history_head - 1 - 1) % 16].cpu));
                    row.set(10, toQ8(server.current.memory - server.history[(server.history_head - 1 - 1) % 16].memory));
                    row.set(11, toQ8(server.current.disk - server.history[(server.history_head - 1 - 1) % 16].disk));
                    row.set(12, toQ8(server.current.network - server.history[(server.history_head - 1 - 1) % 16].network));
                    row.set(13, toQ8(server.current.heat - server.history[(server.history_head - 1 - 1) % 16].heat));
                    row.set(14, toQ8(server.current.errors - server.history[(server.history_head - 1 - 1) % 16].errors));
                    row.set(15, toQ8(server.current.users - server.history[(server.history_head - 1 - 1) % 16].users));
                    
                    // Process k=2 (indices 16-22)
                    row.set(16, toQ8(server.current.cpu - server.history[(server.history_head - 1 - 2) % 16].cpu));
                    row.set(17, toQ8(server.current.memory - server.history[(server.history_head - 1 - 2) % 16].memory));
                    row.set(18, toQ8(server.current.disk - server.history[(server.history_head - 1 - 2) % 16].disk));
                    row.set(19, toQ8(server.current.network - server.history[(server.history_head - 1 - 2) % 16].network));
                    row.set(20, toQ8(server.current.heat - server.history[(server.history_head - 1 - 2) % 16].heat));
                    row.set(21, toQ8(server.current.errors - server.history[(server.history_head - 1 - 2) % 16].errors));
                    row.set(22, toQ8(server.current.users - server.history[(server.history_head - 1 - 2) % 16].users));
                    
                    // Process k=4 (indices 23-29)
                    row.set(23, toQ8(server.current.cpu - server.history[(server.history_head - 1 - 4) % 16].cpu));
                    row.set(24, toQ8(server.current.memory - server.history[(server.history_head - 1 - 4) % 16].memory));
                    row.set(25, toQ8(server.current.disk - server.history[(server.history_head - 1 - 4) % 16].disk));
                    row.set(26, toQ8(server.current.network - server.history[(server.history_head - 1 - 4) % 16].network));
                    row.set(27, toQ8(server.current.heat - server.history[(server.history_head - 1 - 4) % 16].heat));
                    row.set(28, toQ8(server.current.errors - server.history[(server.history_head - 1 - 4) % 16].errors));
                    row.set(29, toQ8(server.current.users - server.history[(server.history_head - 1 - 4) % 16].users));
                    
                    // Process k=8 (indices 30-36)
                    row.set(30, toQ8(server.current.cpu - server.history[(server.history_head - 1 - 8) % 16].cpu));
                    row.set(31, toQ8(server.current.memory - server.history[(server.history_head - 1 - 8) % 16].memory));
                    row.set(32, toQ8(server.current.disk - server.history[(server.history_head - 1 - 8) % 16].disk));
                    row.set(33, toQ8(server.current.network - server.history[(server.history_head - 1 - 8) % 16].network));
                    row.set(34, toQ8(server.current.heat - server.history[(server.history_head - 1 - 8) % 16].heat));
                    row.set(35, toQ8(server.current.errors - server.history[(server.history_head - 1 - 8) % 16].errors));
                    row.set(36, toQ8(server.current.users - server.history[(server.history_head - 1 - 8) % 16].users));
                } else {
                    // For disconnected servers, all change features are 0
                    for (int i = 9; i <= 36; ++i) {
                        row.set(i, int16_t{0});
                    }
                }
                
                // Index 37: ticks since last wager (min((tick - last_probe_tick[s]) & 0xFF, 32) * 8)
                const int ticksSinceWager = ((state.tick - state.seeker.last_probe_tick[s]) & 0xFF);
                row.set(37, static_cast<int16_t>(std::min(ticksSinceWager, 32) * 8));
                
                // Index 38: hider connections destroyed by the last wager (min(belief[s], 8) * 32)
                row.set(38, static_cast<int16_t>(std::min(state.seeker.belief[s], uint8_t{8}) * 32));
            });
        }
    }
    
    // Compute global features
    // Index 0: seeker pool (min(connection_tokens, 32) * 8)
    obs.global.set(0, static_cast<int16_t>(std::min(state.seeker.connection_tokens, uint8_t{32}) * 8));
    
    // Index 1: open seeker connections (popcount(seeker_connection_mask) * 16)
    int popcount = 0;
    for (int i = 0; i < MAX_SERVERS; ++i) {
        if (state.seeker_connection_mask & (1 << i)) {
            popcount++;
        }
    }
    obs.global.set(1, static_cast<int16_t>(popcount * 16));
    
    // Index 2: active servers (count_active_servers * 16)
    obs.global.set(2, static_cast<int16_t>(state.server_count * 16));
    
    // Index 3: tick (min(tick, 256))
    obs.global.set(3, static_cast<int16_t>(std::min(static_cast<uint16_t>(state.tick), static_cast<uint16_t>(256))));
    
    return obs;
}

} // namespace hider_seeker