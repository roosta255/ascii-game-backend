#pragma once

#include "policy_interface.hpp"

namespace hider_seeker {

class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    std::string policyId() const override {
        return "scripted";
    }
    
    void setScores(const Array<SeekerServerScores, MAX_SERVERS>& scores) {
        m_scores = scores;
    }
    
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>& output) const override {
        for (int s = 0; s < MAX_SERVERS; ++s) {
            output.access(s, [&](SeekerServerScores& out) {
                out.open = m_scores.getOrDefault(s, SeekerServerScores{}).open;
                out.close = m_scores.getOrDefault(s, SeekerServerScores{}).close;
                out.wager = m_scores.getOrDefault(s, SeekerServerScores{}).wager;
            });
        }
    }

private:
    Array<SeekerServerScores, MAX_SERVERS> m_scores;
};

} // namespace hider_seeker