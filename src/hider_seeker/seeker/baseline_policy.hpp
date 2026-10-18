#pragma once

#include "policy_interface.hpp"

namespace hider_seeker {

class BaselineSeekerPolicy : public iSeekerPolicy {
public:
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override;
    std::string policyId() const override;
};

} // namespace hider_seeker