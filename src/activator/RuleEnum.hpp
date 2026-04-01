#pragma once

#define RULE_DECL( name_, config_ ) RULE_##name_,
enum RuleEnum
{
#include "Rule.enum"
RULE_COUNT
};
#undef RULE_DECL
