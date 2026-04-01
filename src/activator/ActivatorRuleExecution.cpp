#include "ActivatorRuleExecution.hpp"
#include "RuleFlyweight.hpp"

bool ActivatorRuleExecution::activate(Activation& activation) const {
    return RuleFlyweight::tryActivate(activation);
}
