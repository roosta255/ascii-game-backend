#pragma once

#define EVENT_DECL(name_) EVENT_##name_,
enum EventEnum
{
    EVENT_NIL,
#include "Event.enum"
    EVENT_COUNT
};
#undef EVENT_DECL
