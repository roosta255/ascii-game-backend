#pragma once

enum GeneratorRemodelGroupEnum {
#define GENERATOR_DECL(...)
#define GENERATOR_REMODEL_DECL(name_, ...) REMODEL_GROUP_##name_,
#include "Generator.enum"
#undef GENERATOR_DECL
#undef GENERATOR_REMODEL_DECL
    REMODEL_GROUP_COUNT
};
