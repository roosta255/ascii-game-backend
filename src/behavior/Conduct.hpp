#pragma once

#include "Array.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"
#include "ConductMemoryVariableEnum.hpp"

struct ActivationContext;

struct Conduct {
    int vars[CONDUCT_MEMORY_NPC_VARIABLE_COUNT];
    Array<ConductMemory, CONDUCT_COUNT> memory;
    int characterId = -1;

    Conduct() noexcept : characterId(-1) {
        for (int& v : vars) v = -1;
    }

    int  get(ConductMemoryVariableEnum v) const { return vars[v - CONDUCT_MEMORY_NPC_VARIABLE_BASE]; }
    void set(ConductMemoryVariableEnum v, int val) { vars[v - CONDUCT_MEMORY_NPC_VARIABLE_BASE] = val; }

    bool buildAndExecuteProposals(ActivationContext& ctx);
};
