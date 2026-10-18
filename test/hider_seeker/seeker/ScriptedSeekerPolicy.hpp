#pragma once

#include "policy_interface.hpp"

namespace hider_seeker {

class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    Array<SeekerServerScores, MAX_SERVERS> stored;
    
    void score(const SeekerObservation& observation, Array<SeekerServerScores, MAX_SERVERS>& output) const override {
        output = stored;
    }
    
    std::string policyId() const override {
        return "scripted";
    }
};

} // namespace hider_seeker