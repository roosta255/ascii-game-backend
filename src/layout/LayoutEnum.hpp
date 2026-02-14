#pragma once

#define LAYOUT_DECL( name, x__, y__, z__, t__ ) LAYOUT_##name,
enum LayoutEnum
{
#include "Layout.enum"
LAYOUT_COUNT
};
#undef LAYOUT_DECL
