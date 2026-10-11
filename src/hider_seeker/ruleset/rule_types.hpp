#pragma once

#include <cstdint>

namespace hider_seeker {

// Sentinel meaning "no server" (no relocation target supplied, hider occupies no
// server, or no ring-buffer entry was evicted).
constexpr uint8_t NO_SERVER_ID = 0xFF;

enum class RuleOutcome {
    ACCEPTED,
    REJECTED_NO_SLOT_AVAILABLE,
    REJECTED_INSUFFICIENT_RESOURCE,
    REJECTED_RELOCATION_FAILED,
    REJECTED_INVALID_TARGET
};

} // namespace hider_seeker