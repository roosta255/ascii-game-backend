#pragma once

#include "../../src/hider_seeker/seeker/policy_interface.hpp"

namespace hider_seeker {

// Test-only seeker policy.
class ScriptedSeekerPolicy : public iSeekerPolicy {
public:
    void setScores(const Array<SeekerServerScores, MAX_SERVERS>& s) { stored = s; }
    void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>& out) const override { out = stored; }
    std::string policyId() const override { return "scripted"; }
private:
    Array<SeekerServerScores, MAX_SERVERS> stored{};
};

} // namespace hider_seeker