#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include <cstdint>

namespace hider_seeker {

// Most effects a single HiderAction.enum line may list. A line with more
// fails to compile (see the definition table in action_rules.cpp).
constexpr uint8_t MAX_ACTION_EFFECTS = 4;

struct ActionEffect {
    uint8_t metric; // a ServerMetric
    int16_t delta;  // signed; the result is clamped to the metric's [min, max]
};

struct HiderActionDef {
    uint16_t ap_cost;
    Array<ActionEffect, MAX_ACTION_EFFECTS> effects; // fixed-capacity, no dynamic memory
    uint8_t effect_count;                            // effects[0, effect_count) are used
};

// Generated from HiderAction.enum: one enumerator per HIDER_ACTION_DECL line,
// in file order. The numeric index is the save format (PendingMove::server_mode).
enum HiderActionMode : uint8_t {
#define HIDER_ACTION_DECL(name, ap_cost, effects) name,
#include "HiderAction.enum"
#undef HIDER_ACTION_DECL
    HIDER_ACTION_MODE_COUNT
};

static_assert(ADD_SERVER == 0 && REMOVE_SERVER == 1,
              "ADD_SERVER and REMOVE_SERVER indices are the save format and must stay 0 and 1");

// Callers are expected to pass in-range modes only; an out-of-range mode
// yields a definition no hider can afford, so it is never applied.
const HiderActionDef& hiderActionDef(HiderActionMode mode);
// nullptr when mode >= HIDER_ACTION_MODE_COUNT.
const char* hiderActionName(HiderActionMode mode);
// Exact, case-sensitive match against the HiderAction.enum names; false (and
// out untouched) when there is no match.
bool hiderActionFromName(const char* text, HiderActionMode& out);

// Applies hiderId's action mode using only the backend's definition table
// (never values from a submitted intent). Rejects with
// REJECTED_INSUFFICIENT_RESOURCE and changes nothing when the hider cannot
// pay def.ap_cost; otherwise pays the cost and applies the effects to the
// hider's occupied server if it is active.
RuleOutcome applyHiderAction(MatchState& match, uint8_t hiderId, HiderActionMode mode);

} // namespace hider_seeker
