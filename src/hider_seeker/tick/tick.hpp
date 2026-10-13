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

RuleOutcome applyTickIntent(MatchState& state, const TickIntent& intent);
TickOutput runTick(MatchState& state,
                    const Array<TickIntent, MAX_INTENTS_PER_TICK>& candidateIntents,
                    uint8_t candidateIntentCount,
                    const ConsumedKeySet& alreadyConsumed,
                    const iTargetRankingPolicy* seekerPolicy);

} // namespace hider_seeker