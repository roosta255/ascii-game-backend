#pragma once

#include "policy_interface.hpp"

namespace hider_seeker {

class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    Array<SeekerServerScores, MAX_SERVERS> scores;
    
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const override;
    std::string policyId() const override;
};

} // namespace hider_seeker