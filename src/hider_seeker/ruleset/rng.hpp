#pragma once

#include <cstdint>

namespace hider_seeker {

struct RngState {
    uint64_t state;
    uint64_t counter;
};

uint32_t nextRandomValue(RngState&);

} // namespace hider_seeker