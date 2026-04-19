#pragma once

#include "ActivationContext.hpp"
#include "Array.hpp"
#include "RuleEnum.hpp"
#include "WrapperConfig.hpp"

struct RuleFlyweight {
    const char* name = nullptr;
    WrapperConfig config = {};

    static const Array<RuleFlyweight, RULE_COUNT>& getFlyweights();

    // Iterates all global rules in priority order (highest first).
    // Returns true if a matching rule activated successfully.
    // Returns false and leaves a failure in the codeset if a rule matched but failed,
    // or if no rule matched (CODE_RULE_EXECUTION_FAILED).
    static bool tryActivate(ActivationContext& activation);
};
