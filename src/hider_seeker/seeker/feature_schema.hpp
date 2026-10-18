#pragma once

#include <cstdint>
#include "state.hpp"
#include "../../data/Array.hpp"

namespace hider_seeker {

constexpr uint8_t SEEKER_FEATURE_VERSION = 1;
constexpr uint8_t SEEKER_SERVER_FEATURES = 39;
constexpr uint8_t SEEKER_GLOBAL_FEATURES = 4;
constexpr uint8_t SEEKER_INPUT_SIZE = 43; // server features then global features

struct SeekerObservation {
    Array<Array<int16_t, SEEKER_SERVER_FEATURES>, MAX_SERVERS> server;
    Array<int16_t, SEEKER_GLOBAL_FEATURES> global;
};

} // namespace hider_seeker