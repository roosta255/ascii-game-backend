#pragma once

#include "policy_interface.hpp"

namespace hider_seeker {

class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    Array<SeekerServerScores, MAX_SERVERS> scriptedScores;
    
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>& output) const override {
        output = scriptedScores;
    }
    
    std::string policyId() const override {
        return "scripted";
    }
};

} // namespace hider_seeker