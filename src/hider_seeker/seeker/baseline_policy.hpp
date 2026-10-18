#pragma once

#include "policy_interface.hpp"

class BaselineSeekerPolicy : public iSeekerPolicy {
public:
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override;
    std::string policyId() const override;
};