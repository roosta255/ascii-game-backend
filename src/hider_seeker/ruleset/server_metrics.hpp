#pragma once

#include "../domain/state.hpp"
#include "../../data/Pointer.hpp"
#include <cstdint>

namespace hider_seeker {

// Generated from ServerMetric.enum: one SERVER_METRIC_<name> enumerator per
// SERVER_METRIC_DECL line, in file order.
enum ServerMetric : uint8_t {
#define SERVER_METRIC_DECL(name, min_value, max_value, rest_value, drift_per_tick) SERVER_METRIC_##name,
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL
    SERVER_METRIC_COUNT
};

static_assert(SERVER_METRIC_COUNT == 8, "ServerMetric.enum must declare exactly eight metrics");

// Every ServerMetric.enum line must satisfy min <= rest <= max and fit uint8_t.
#define SERVER_METRIC_DECL(name, min_value, max_value, rest_value, drift_per_tick)            \
    static_assert((min_value) <= (rest_value) && (rest_value) <= (max_value),                 \
                  "ServerMetric.enum: " #name " must satisfy min <= rest <= max");            \
    static_assert((max_value) <= UINT8_MAX && (drift_per_tick) <= UINT8_MAX,                  \
                  "ServerMetric.enum: " #name " values must fit in uint8_t");
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL

struct ServerMetricDef {
    uint8_t min;
    uint8_t max;
    uint8_t rest;
    uint8_t drift_per_tick;
};

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