#pragma once

#include <cstdint>
#include "../domain/state.hpp"
#include "../../data/Array.hpp"

namespace hider_seeker {

constexpr uint8_t SEEKER_FEATURE_VERSION = 1;
constexpr uint8_t SEEKER_SERVER_FEATURES = 39;
constexpr uint8_t SEEKER_GLOBAL_FEATURES = 4;
constexpr uint8_t SEEKER_INPUT_SIZE = 43; // server features then global features

/**
 * Server feature table (indices 0-38):
 * 
 * Index | Feature                    | Value
 * ------|----------------------------|------
 * 0     | active                     | 256 / 0
 * 1     | seeker connected           | 256 / 0
 * 2-8   | current cpu, memory, disk, network, heat, errors, users | Q8 of `current`; 0 unless connected
 * 9-36  | change over k = 1, 2, 4, 8 ticks, for each of the 7 metrics (k-major: indices 9-15 are k=1, 16-22 are k=2, and so on) | Q8 of `current.m - frameAgo(s, k).m` (signed); 0 unless connected
 * 37    | ticks since last wager     | min((tick - last_probe_tick[s]) & 0xFF, 32) * 8
 * 38    | hider connections destroyed by the last wager | min(belief[s], 8) * 32
 *
 * Global feature table (indices 0-3):
 * 
 * Index | Feature                    | Value
 * ------|----------------------------|------
 * 0     | seeker pool                | min(connection_tokens, 32) * 8
 * 1     | open seeker connections    | popcount(mask) * 16
 * 2     | active servers             | count * 16
 * 3     | tick                       | min(tick, 256)
 */
struct SeekerObservation {
    Array<Array<int16_t, SEEKER_SERVER_FEATURES>, MAX_SERVERS> server;
    Array<int16_t, SEEKER_GLOBAL_FEATURES> global;
};

} // namespace hider_seeker