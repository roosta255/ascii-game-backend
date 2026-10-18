#pragma once

#include "state.hpp"
#include "feature_schema.hpp"

namespace hider_seeker {

SeekerObservation buildSeekerObservation(const MatchState&);

} // namespace hider_seeker