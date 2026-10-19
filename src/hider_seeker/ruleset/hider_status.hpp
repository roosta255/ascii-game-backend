#pragma once
#include <cstdint>

namespace hider_seeker {

enum HiderStatus : uint8_t {
    HIDER_STATUS_ACTIVE = 0,
    HIDER_STATUS_CAPTURED = 1
};

} // namespace hider_seeker