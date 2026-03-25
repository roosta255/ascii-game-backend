#pragma once

// Each EVENT_DECL generates two consecutive enum values: SOURCE then RESULT.
// RESULT is always SOURCE + 1 — enforced by layout, not a runtime check.
#define EVENT_DECL(name_, d0_, d1_, d2_) EVENT_##name_##_SOURCE, EVENT_##name_##_RESULT,
enum EventEnum
{
    EVENT_NIL,
#include "Event.enum"
    EVENT_COUNT
};
#undef EVENT_DECL
