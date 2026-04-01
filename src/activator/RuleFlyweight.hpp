#pragma once

#include "Activation.hpp"
#include "Array.hpp"
#include "RuleEnum.hpp"
#include "WrapperConfig.hpp"

struct RuleFlyweight {
    const char* name = nullptr;
    WrapperConfig config = {};

    static const Array<RuleFlyweight, RULE_COUNT>& getFlyweights();

    // Iterates all global rules in priority order (highest first).
    // Returns true if a matching rule activated successfully.
    // Returns false and leaves a failure in the codeset if a rule matched but failed.
    // Returns false with no codeset change if no rule matched.
    static bool tryActivate(Activation& activation);
};
