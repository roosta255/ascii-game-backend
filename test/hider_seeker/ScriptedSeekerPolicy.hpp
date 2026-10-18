#pragma once

#include "../../src/hider_seeker/seeker/policy_interface.hpp"

namespace hider_seeker {

class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    Array<SeekerServerScores, MAX_SERVERS> storedScores;
    
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>& out) const override {
        out = storedScores;
    }
    
    std::string policyId() const override {
        return "scripted";
    }
};

} // namespace hider_seeker