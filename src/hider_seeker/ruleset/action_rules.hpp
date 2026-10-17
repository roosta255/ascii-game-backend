#pragma once

#include "rule_types.hpp"
#include "../domain/state.hpp"
#include "../../data/Array.hpp"
#include <cstdint>

namespace hider_seeker {

// Most effects a single HiderAction.enum line may list. A line with more
// fails to compile (see the definition table in action_rules.cpp).
constexpr uint8_t MAX_ACTION_EFFECTS = 4;

// metric value of an unused effect slot (no ServerMetric has this index).
constexpr uint8_t NO_EFFECT_METRIC = 0xFF;

struct ActionEffect {
    uint8_t metric = NO_EFFECT_METRIC; // a ServerMetric
    int16_t delta = 0;                 // signed; the result is clamped to the metric's [min, max]
};

// One HiderAction.enum line's fields. Listing more than MAX_ACTION_EFFECTS
// effects is a "too many initializers" compile error, never a truncation.
struct HiderActionConfig {
    uint16_t ap_cost = 0;
    // A plain array so a line can write .effects = { {A, 1}, {B, 2} }.
    ActionEffect effects[MAX_ACTION_EFFECTS] = {};
};

struct HiderActionDef {
    uint16_t ap_cost;
    Array<ActionEffect, MAX_ACTION_EFFECTS> effects; // fixed-capacity, no dynamic memory
    uint8_t effect_count;                            // effects[0, effect_count) are used
};

// Generated from HiderAction.enum: one enumerator per HIDER_ACTION_DECL line,
// in file order. The numeric index is the save format (PendingMove::server_mode).
enum class HiderActionMode : uint8_t {
#define HIDER_ACTION_DECL(name, ...) name,
#include "HiderAction.enum"
#undef HIDER_ACTION_DECL
};

// The number of HIDER_ACTION_DECL lines.
constexpr uint8_t HIDER_ACTION_MODE_COUNT = 0
#define HIDER_ACTION_DECL(name, ...) + 1
#include "HiderAction.enum"
#undef HIDER_ACTION_DECL
    ;

static_assert(static_cast<uint8_t>(HiderActionMode::ADD_SERVER) == 0 &&
                  static_cast<uint8_t>(HiderActionMode::REMOVE_SERVER) == 1,
              "ADD_SERVER and REMOVE_SERVER indices are the save format and must stay 0 and 1");

static_assert(static_cast<uint8_t>(HiderActionMode::CONNECT) == 7);
static_assert(static_cast<uint8_t>(HiderActionMode::DISCONNECT) == 8);

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
