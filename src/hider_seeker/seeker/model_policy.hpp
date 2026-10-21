#pragma once

#include "policy_interface.hpp"
#include "seeker_model.hpp"

namespace hider_seeker {

class ModelSeekerPolicy : public iSeekerPolicy {
public:
    ModelSeekerPolicy(SeekerModel model, std::string modelVersion);
    void score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& out) const override;
    std::string policyId() const override;
private:
    SeekerModel model_;
    std::string modelVersion_;
};

} // namespace hider_seeker