#pragma once

#include <cstdint>
#include "Array.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

constexpr uint8_t SEEKER_FEATURE_VERSION = 1;
constexpr uint8_t SEEKER_SERVER_FEATURES = 39;
constexpr uint8_t SEEKER_GLOBAL_FEATURES = 4;
constexpr uint8_t SEEKER_INPUT_SIZE = 43; // server features then global features

/**
 * Server feature table (indices 0-38):
 * 
 * Index 0: active - 256 if server is active, else 0
 * Index 1: seeker connected - 256 if seeker is connected to server s, else 0
 * Indices 2-8: current metrics (cpu, memory, disk, network, heat, errors, users) - Q8 values when connected, otherwise 0
 * Indices 9-36: change over k = 1, 2, 4, 8 ticks for each of the 7 metrics (k-major layout)
 *   - k=1 block (indices 9-15): cpu, memory, disk, network, heat, errors, users
 *   - k=2 block (indices 16-22): cpu, memory, disk, network, heat, errors, users  
 *   - k=4 block (indices 23-29): cpu, memory, disk, network, heat, errors, users
 *   - k=8 block (indices 30-36): cpu, memory, disk, network, heat, errors, users
 *   - All values are Q8 of current.m - frameAgo(s,k).m when connected, otherwise 0
 * Index 37: ticks since last wager - min((tick - last_probe_tick[s]) & 0xFF, 32) * 8
 * Index 38: hider connections destroyed by last wager - min(belief[s], 8) * 32
 *
 * Global feature table (indices 0-3):
 * Index 0: seeker pool - min(connection_tokens, 32) * 8
 * Index 1: open seeker connections - popcount(seeker_connection_mask) * 16
 * Index 2: active servers - count_active_servers * 16
 * Index 3: tick - min(tick, 256)
 */
struct SeekerObservation {
    Array<Array<int16_t, SEEKER_SERVER_FEATURES>, MAX_SERVERS> server;
    Array<int16_t, SEEKER_GLOBAL_FEATURES> global;
};

} // namespace hider_seeker