#pragma once

#define REMODEL_DECL( name_, class_, is_blocker_ ) REMODEL_##name_,
enum RemodelEnum
{
#include "Remodel.enum"
REMODEL_COUNT
};
#undef REMODEL_DECL
