#pragma once

namespace hider_seeker {

enum class RuleOutcome {
    ACCEPTED,
    REJECTED_NO_SLOT_AVAILABLE,
    REJECTED_INSUFFICIENT_RESOURCE,
    REJECTED_RELOCATION_FAILED,
    REJECTED_INVALID_TARGET
};

} // namespace hider_seeker