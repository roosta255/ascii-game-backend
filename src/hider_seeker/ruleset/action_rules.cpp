#include "action_rules.hpp"
#include "server_metrics.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace hider_seeker {

namespace {

#define HIDER_ACTION_UNPAREN(...) __VA_ARGS__

// Generated from HiderAction.enum, indexed by HiderActionMode. Each line's
// effects initialise a std::array of exactly MAX_ACTION_EFFECTS slots, so a
// line listing more effects is a "too many initializers" compile error
// rather than a silent truncation.
const Array<HiderActionDef, HIDER_ACTION_MODE_COUNT> HIDER_ACTION_DEFS(std::array<HiderActionDef, HIDER_ACTION_MODE_COUNT>{{
#define HIDER_ACTION_DECL(name, ap_cost, effects)                                                         \
    HiderActionDef{                                                                                        \
        ap_cost,                                                                                           \
        Array<ActionEffect, MAX_ACTION_EFFECTS>(                                                           \
            std::array<ActionEffect, MAX_ACTION_EFFECTS>{{HIDER_ACTION_UNPAREN effects}}),                 \
        static_cast<uint8_t>(std::initializer_list<ActionEffect>{HIDER_ACTION_UNPAREN effects}.size())},
#include "HiderAction.enum"
#undef HIDER_ACTION_DECL
}});

// Generated from HiderAction.enum, indexed by HiderActionMode.
const Array<const char*, HIDER_ACTION_MODE_COUNT> HIDER_ACTION_NAMES(std::array<const char*, HIDER_ACTION_MODE_COUNT>{{
#define HIDER_ACTION_DECL(name, ap_cost, effects) #name,
#include "HiderAction.enum"
#undef HIDER_ACTION_DECL
}});

#undef HIDER_ACTION_UNPAREN

// Returned for an out-of-range mode: no hider can afford it, so it is never applied.
const HiderActionDef UNAFFORDABLE_ACTION_DEF{UINT16_MAX, Array<ActionEffect, MAX_ACTION_EFFECTS>(), 0};

} // namespace

const HiderActionDef& hiderActionDef(HiderActionMode mode) {
    return HIDER_ACTION_DEFS.getOrDefault(static_cast<int>(mode), UNAFFORDABLE_ACTION_DEF);
}

const char* hiderActionName(HiderActionMode mode) {
    const char* name = nullptr;
    HIDER_ACTION_NAMES.accessConst(static_cast<int>(mode), [&name](const char* const& entry) { name = entry; });
    return name;
}

bool hiderActionFromName(const char* text, HiderActionMode& out) {
    if (text == nullptr) {
        return false;
    }
    // The index is the HiderActionMode value being looked up.
    for (int index = 0; index < HIDER_ACTION_MODE_COUNT; ++index) {
        const char* name = hiderActionName(static_cast<HiderActionMode>(index));
        if (name != nullptr && std::strcmp(text, name) == 0) {
            out = static_cast<HiderActionMode>(index);
            return true;
        }
    }
    return false;
}

RuleOutcome applyHiderAction(MatchState& match, uint8_t hiderId, HiderActionMode mode) {
    if (hiderId >= MAX_HIDERS) {
        return RuleOutcome::REJECTED_INVALID_TARGET;
    }
    HiderState& hider = match.hiders[hiderId];

    // Cost and effects come only from the backend's own definition table.
    const HiderActionDef& def = hiderActionDef(mode);
    if (hider.action_points < def.ap_cost) {
        return RuleOutcome::REJECTED_INSUFFICIENT_RESOURCE;
    }
    hider.action_points = static_cast<uint8_t>(hider.action_points - def.ap_cost);

    // No occupied server, or an inactive one: the AP is paid and nothing else changes.
    if (hider.server_id >= MAX_SERVERS || !match.servers[hider.server_id].active) {
        return RuleOutcome::ACCEPTED;
    }
    ServerState& server = match.servers[hider.server_id];

    const uint8_t traceBefore = serverMetricField(server, SERVER_METRIC_TRACE).orElse(0);
    // Only the first effect_count slots are used.
    for (int i = 0; i < def.effect_count; ++i) {
        def.effects.accessConst(i, [&server](const ActionEffect& effect) {
            const ServerMetric metric = static_cast<ServerMetric>(effect.metric);
            applyMetricDelta(server, metric, effect.delta, serverMetricDef(metric));
        });
    }
    const uint8_t traceAfter = serverMetricField(server, SERVER_METRIC_TRACE).orElse(0);

    // An action that raised trace makes it fresh again.
    if (traceAfter > traceBefore) {
        server.trace_age = 0;
    }

    return RuleOutcome::ACCEPTED;
}

} // namespace hider_seeker
