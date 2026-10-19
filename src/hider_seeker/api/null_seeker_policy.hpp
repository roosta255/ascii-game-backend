#pragma once

#include <string>
#include "../seeker/policy_interface.hpp"

namespace hider_seeker {

// PLACEHOLDER pending SEEKER-005: a seeker policy that leaves every score at
// 0, so the seeker never opens, closes or wagers.
class NullSeekerPolicy : public iSeekerPolicy {
public:
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override {}
    std::string policyId() const override { return "null"; }
};

} // namespace hider_seeker