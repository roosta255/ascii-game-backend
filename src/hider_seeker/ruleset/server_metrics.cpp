#include "server_metrics.hpp"
#include "../../data/Array.hpp"
#include <algorithm>
#include <array>

namespace hider_seeker {

namespace {

// Generated from ServerMetric.enum, indexed by ServerMetric.
const Array<ServerMetricDef, SERVER_METRIC_COUNT> SERVER_METRIC_DEFS(std::array<ServerMetricDef, SERVER_METRIC_COUNT>{{
#define SERVER_METRIC_DECL(name, min_value, max_value, rest_value, drift_per_tick) \
    ServerMetricDef{min_value, max_value, rest_value, drift_per_tick},
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL
}});

// Generated from ServerMetric.enum, indexed by ServerMetric.
const Array<const char*, SERVER_METRIC_COUNT> SERVER_METRIC_NAMES(std::array<const char*, SERVER_METRIC_COUNT>{{
#define SERVER_METRIC_DECL(name, min_value, max_value, rest_value, drift_per_tick) #name,
#include "ServerMetric.enum"
#undef SERVER_METRIC_DECL
}});

// Returned for an out-of-range metric: leaves any value unchanged.
const ServerMetricDef NO_OP_METRIC_DEF{0, UINT8_MAX, 0, 0};

uint8_t clampToDef(int value, const ServerMetricDef& def) {
    return static_cast<uint8_t>(std::clamp(value, static_cast<int>(def.min), static_cast<int>(def.max)));
}

} // namespace

const ServerMetricDef& serverMetricDef(ServerMetric metric) {
    return SERVER_METRIC_DEFS.getOrDefault(static_cast<int>(metric), NO_OP_METRIC_DEF);
}

const char* serverMetricName(ServerMetric metric) {
    if (!SERVER_METRIC_NAMES.containsIndex(static_cast<int>(metric))) {
        return nullptr;
    }
    const char* name = nullptr;
    SERVER_METRIC_NAMES.accessConst(static_cast<int>(metric), [&name](const char* const& entry) { name = entry; });
    return name;
}

Pointer<uint8_t> serverMetricField(ServerState& server, ServerMetric metric) {
    switch (metric) {
        case SERVER_METRIC_CPU:     return Pointer<uint8_t>(server.current.cpu);
        case SERVER_METRIC_MEMORY:  return Pointer<uint8_t>(server.current.memory);
        case SERVER_METRIC_DISK:    return Pointer<uint8_t>(server.current.disk);
        case SERVER_METRIC_NETWORK: return Pointer<uint8_t>(server.current.network);
        case SERVER_METRIC_HEAT:    return Pointer<uint8_t>(server.current.heat);
        case SERVER_METRIC_ERRORS:  return Pointer<uint8_t>(server.current.errors);
        case SERVER_METRIC_USERS:   return Pointer<uint8_t>(server.current.users);
        case SERVER_METRIC_TRACE:   return Pointer<uint8_t>(server.trace_strength);
        case SERVER_METRIC_COUNT:   break;
    }
    return Pointer<uint8_t>();
}

void applyMetricDelta(ServerState& server, ServerMetric metric, int16_t delta, const ServerMetricDef& def) {
    serverMetricField(server, metric).access([&](uint8_t& value) {
        value = clampToDef(static_cast<int>(value) + delta, def);
    });
}

void driftMetric(ServerState& server, ServerMetric metric, const ServerMetricDef& def) {
    serverMetricField(server, metric).access([&](uint8_t& value) {
        const int rest = def.rest;
        int next = value;
        if (next > rest) {
            next = std::max(next - def.drift_per_tick, rest);
        } else if (next < rest) {
            next = std::min(next + def.drift_per_tick, rest);
        }
        value = clampToDef(next, def);
    });
}

} // namespace hider_seeker
