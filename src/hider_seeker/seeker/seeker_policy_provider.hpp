#pragma once

#include <string>
#include <memory>
#include "policy_interface.hpp"
#include "model_policy.hpp"
#include "baseline_policy.hpp"

namespace hider_seeker {

class SeekerPolicyProvider {
public:
    explicit SeekerPolicyProvider(const std::string& root);
    const iSeekerPolicy& policy() const;
    const std::string& fallbackReason() const;
private:
    BaselineSeekerPolicy baseline_;
    std::unique_ptr<ModelSeekerPolicy> model_;
    std::string fallbackReason_;
};

} // namespace hider_seeker