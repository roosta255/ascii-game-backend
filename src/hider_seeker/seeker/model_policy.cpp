#include "model_policy.hpp"
#include "feature_schema.hpp"

namespace hider_seeker {

ModelSeekerPolicy::ModelSeekerPolicy(SeekerModel model, std::string modelVersion) 
    : model_(model), modelVersion_(modelVersion) {}

void ModelSeekerPolicy::score(const SeekerObservation& obs, Array<SeekerServerScores, MAX_SERVERS>& out) const {
    // Process each server
    for (int s = 0; s < MAX_SERVERS; ++s) {
        // Only process active servers (feature 0 is non-zero)
        int16_t isActive = obs.server.getOrDefault(s, Array<int16_t, SEEKER_SERVER_FEATURES>{}).getOrDefault(0, int16_t{0});
        
        if (isActive == 0) {
            // Server is inactive, set scores to zero
            out.access(s, [&](SeekerServerScores& scores) {
                scores.open = 0;
                scores.close = 0;
                scores.wager = 0;
            });
            continue;
        }
        
        // Build input vector: server features + global features
        Array<int16_t, SEEKER_MODEL_INPUTS> input;
        int inputIndex = 0;
        
        // Copy server features (39 features)
        const auto& serverFeatures = obs.server.getOrDefault(s, Array<int16_t, SEEKER_SERVER_FEATURES>{});
        for (int i = 0; i < SEEKER_SERVER_FEATURES; ++i) {
            input.assignValue(inputIndex++, serverFeatures.getOrDefault(i, int16_t{0}));
        }
        
        // Copy global features (4 features)
        for (int i = 0; i < SEEKER_GLOBAL_FEATURES; ++i) {
            input.assignValue(inputIndex++, obs.global.getOrDefault(i, int16_t{0}));
        }
        
        // Run the model
        Array<int32_t, SEEKER_MODEL_OUTPUTS> output;
        runSeekerModel(model_, input, output);
        
        // Store the results - we need to access each element directly since we can't use lambda with accessConst
        int32_t open = output.getOrDefault(0, int32_t{0});
        int32_t close = output.getOrDefault(1, int32_t{0});
        int32_t wager = output.getOrDefault(2, int32_t{0});
        
        out.access(s, [&](SeekerServerScores& scores) {
            scores.open = open;
            scores.close = close;
            scores.wager = wager;
        });
    }
}

std::string ModelSeekerPolicy::policyId() const {
    return "model:" + modelVersion_;
}

} // namespace hider_seeker