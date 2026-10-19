#pragma once

#include "../../hider_seeker/domain/state.hpp"
#include "../ruleset/rule_types.hpp"
#include "../ruleset/action_rules.hpp"
#include "../ruleset/intent_ordering.hpp"
#include "../ruleset/probe_rules.hpp"
#include "../ruleset/ring_buffers.hpp"
#include "../ruleset/target_ranking_interface.hpp"
#include "tick_types.hpp"

namespace hider_seeker {

// The single per-TickIntent dispatch point runTick uses for every intent.
// Applies exactly one intent to state; the 4-argument form appends any public
// lifecycle event to outPublicEvents, the 2-argument form discards it. Neither
// touches the seeker policy, probes, or metric frames.
RuleOutcome applyTickIntent(MatchState& state, const TickIntent& intent,
                            Array<PublicLifecycleEvent, MAX_TICK_PUBLIC_EVENTS>& outPublicEvents,
                            uint8_t& outPublicEventCount);
RuleOutcome applyTickIntent(MatchState& state, const TickIntent& intent);
TickOutput runTick(MatchState& state,
                    const Array<TickIntent, MAX_INTENTS_PER_TICK>& candidateIntents,
                    uint8_t candidateIntentCount,
                    const ConsumedKeySet& alreadyConsumed,
                    const iTargetRankingPolicy* seekerPolicy);

} // namespace hider_seeker