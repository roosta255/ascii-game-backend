#pragma once

#include <cstdint>
#include "../domain/state.hpp"
#include "../data/Array.hpp"

namespace hider_seeker {

// Seeker observation feature schema version
constexpr uint8_t SEEKER_FEATURE_VERSION = 1;
constexpr uint8_t SEEKER_SERVER_FEATURES = 39;
constexpr uint8_t SEEKER_GLOBAL_FEATURES = 4;
constexpr uint8_t SEEKER_INPUT_SIZE = 43; // server features then global features

// Server feature table (indices 0-38):
// Index 0: active (256 if server is active, else 0)
// Index 1: seeker connected (256 if seeker is connected to this server, else 0)
// Indices 2-8: Q8 values of current cpu, memory, disk, network, heat, errors, users when connected; otherwise 0
// Indices 9-36: Q8 of signed change current.m - frameAgo(s, k).m for each metric m and each k-major (k=1 indices 9-15, k=2 indices 16-22, k=4 indices 23-29, k=8 indices 30-36) when connected; otherwise 0
// Index 37: ticks since last wager (min((tick - last_probe_tick[s]) & 0xFF, 32) * 8)
// Index 38: hider connections destroyed by the last wager (min(belief[s], 8) * 32)

// Global feature table (indices 0-3):
// Index 0: seeker pool (min(connection_tokens, 32) * 8)
// Index 1: open seeker connections (popcount(seeker_connection_mask) * 16)
// Index 2: active servers (count_active_servers * 16)
// Index 3: tick (min(tick, 256))

struct SeekerObservation {
    Array<Array<int16_t, SEEKER_SERVER_FEATURES>, MAX_SERVERS> server;
    Array<int16_t, SEEKER_GLOBAL_FEATURES> global;
};

} // namespace hider_seeker