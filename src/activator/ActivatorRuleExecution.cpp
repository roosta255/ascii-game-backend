#include "ActivatorRuleExecution.hpp"
#include "RuleFlyweight.hpp"

bool ActivatorRuleExecution::activate(ActivationContext& ctx) const {
    return RuleFlyweight::tryActivate(ctx);
}
