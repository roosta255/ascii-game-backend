#pragma once

#include "Array.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"

struct ActivationContext;

struct Conduct {
    Array<ConductMemory, CONDUCT_COUNT> memory;
    int characterId = -1;

    bool buildAndExecuteProposals(ActivationContext& ctx);
};
