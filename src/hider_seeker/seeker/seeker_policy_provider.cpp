#include "seeker_policy_provider.hpp"
#include "model_store.hpp"
#include "seeker_model.hpp"
#include <iostream>
#include <memory>

namespace hider_seeker {

SeekerPolicyProvider::SeekerPolicyProvider(const std::string& root) : baseline_() {
    SeekerModel model;
    std::string version;
    std::string error;
    
    if (loadActiveSeekerModel(root, model, version, error)) {
        // Successfully loaded the model
        model_ = std::make_unique<ModelSeekerPolicy>(model, version);
        fallbackReason_.clear();
    } else {
        // Failed to load the model, use baseline policy
        model_.reset(nullptr);
        fallbackReason_ = error;
    }
}

const iSeekerPolicy& SeekerPolicyProvider::policy() const {
    if (model_) {
        return *model_;
    }
    return baseline_;
}

const std::string& SeekerPolicyProvider::fallbackReason() const {
    return fallbackReason_;
}

} // namespace hider_seeker