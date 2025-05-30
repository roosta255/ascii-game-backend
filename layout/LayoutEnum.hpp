#pragma once

#define LAYOUT_DECL( name ) LAYOUT_##name,
enum LayoutEnum
{
#include "Layout.enum"
LAYOUT_COUNT
};
#undef LAYOUT_DECL
