#pragma once

#define CONDUCT_DECL(name_) CONDUCT_##name_,
enum ConductEnum {
#include "Conduct.enum"
    CONDUCT_COUNT
};
#undef CONDUCT_DECL
