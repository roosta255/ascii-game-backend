#pragma once

#include "policy_interface.hpp"

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