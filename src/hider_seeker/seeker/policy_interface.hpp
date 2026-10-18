#pragma once

#include "feature_schema.hpp"
#include "../../data/Array.hpp"
#include <string>

// MAX_SERVERS is needed for the template parameter
#include "../domain/state.hpp"

struct SeekerServerScores { 
    int32_t open = 0; 
    int32_t close = 0; 
    int32_t wager = 0; 
}; // Q8

class iSeekerPolicy {
public:
    virtual void score(const SeekerObservation&, Array<SeekerServerScores, MAX_SERVERS>&) const = 0;
    virtual std::string policyId() const = 0; // "null", "baseline", "model:<version>"
    virtual ~iSeekerPolicy() = default;
};