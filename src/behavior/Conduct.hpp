#pragma once

#include "Array.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"

struct ActivationContext;

struct Conduct {
    Array<int, CONDUCT_MEMORY_NPC_VARIABLE_COUNT> vars;
    Array<ConductMemory, CONDUCT_COUNT> memory;
    int characterId = -1;

    Conduct() noexcept : characterId(-1) {
        for (int& v : vars) v = -1;
    }

    int  get(ConductMemoryVariableEnum v) const { return vars.getOrDefault(v - CONDUCT_MEMORY_NPC_VARIABLE_BASE, -1); }
    void set(ConductMemoryVariableEnum v, int val) { vars.assignValue(v - CONDUCT_MEMORY_NPC_VARIABLE_BASE, val); }

    bool buildAndExecuteProposals(ActivationContext& ctx);
};
