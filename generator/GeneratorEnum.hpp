#pragma once

#define GENERATOR_DECL( name_, class_ ) GENERATOR_##name_,
enum GeneratorEnum
{
#include "Generator.enum"
GENERATOR_COUNT
};
#undef GENERATOR_DECL
