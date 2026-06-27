#pragma once

#include "ProposalEffect.hpp"

// Data-driven replacement for iProposer. Stored directly in BehaviorFlyweight.
// When multiple conducts have an active proposer, the highest-priority one wins.
struct ProposerConfig {
    static constexpr int MAX_EFFECTS = 8;
    int            priority = 0;
    ProposalEffect effects[MAX_EFFECTS] = {};
};
