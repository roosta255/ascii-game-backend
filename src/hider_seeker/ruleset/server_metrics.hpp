#pragma once

#include "../domain/state.hpp"
#include "../../data/Pointer.hpp"
#include <cstdint>

namespace hider_seeker {

// One ServerMetric.enum line's fields. A value outside 0..255 is a
// narrowing error at compile time.
struct ServerMetricDef {
    uint8_t min = 0;
    uint8_t max = 0;
    uint8_t rest = 0;
    uint8_t drift_per_tick = 0;
};

constexpr bool isValidServerMetricDef(const ServerMetricDef& def) {
    return def.min <= def.rest && def.rest <= def.max;
}

// Generated from ServerMetric.enum: one SERVER_METRIC_<NAME> enumerator per
// SERVER_METRIC_DECL line, in file order.
enum ServerMetric : uint8_t {
#define SERVER_METRIC_DECL(name, ...) SERVER_METRIC_##name,
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL
    SERVER_METRIC_COUNT
};

static_assert(SERVER_METRIC_COUNT == 8, "ServerMetric.enum must declare exactly eight metrics");

// Every ServerMetric.enum line must satisfy min <= rest <= max.
#define SERVER_METRIC_DECL(name, ...)                                                         \
    static_assert(isValidServerMetricDef(ServerMetricDef __VA_ARGS__),                      \
                  "ServerMetric.enum: " #name " must satisfy min <= rest <= max");
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL

// Callers are expected to pass in-range metrics only; an out-of-range metric
// yields a no-op definition (full uint8_t range, no drift).
const ServerMetricDef& serverMetricDef(ServerMetric metric);
// "CPU" .. "TRACE"; nullptr when metric >= SERVER_METRIC_COUNT.
const char* serverMetricName(ServerMetric metric);
// Points into the matching MetricFrame field of server.current for
// CPU..USERS, or into server.trace_strength for TRACE; empty when out of range.
Pointer<uint8_t> serverMetricField(ServerState& server, ServerMetric metric);
// result = clamp(current + delta) into [def.min, def.max].
void applyMetricDelta(ServerState& server, ServerMetric metric, int16_t delta, const ServerMetricDef& def);
// Moves current toward def.rest by def.drift_per_tick, never past rest, then
// clamps to [def.min, def.max].
void driftMetric(ServerState& server, ServerMetric metric, const ServerMetricDef& def);

} // namespace hider_seeker