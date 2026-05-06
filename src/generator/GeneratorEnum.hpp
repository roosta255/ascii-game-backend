#pragma once

#define GENERATOR_DECL( name_, class_, isTest_, ... ) GENERATOR_##name_,
#define GENERATOR_REMODEL_DECL(...)
enum GeneratorEnum
{
#include "Generator.enum"
GENERATOR_COUNT
};
#undef GENERATOR_DECL
#undef GENERATOR_REMODEL_DECL
