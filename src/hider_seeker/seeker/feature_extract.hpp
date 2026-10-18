#pragma once

#include "feature_schema.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

// Declares the function that builds a SeekerObservation from a MatchState.
SeekerObservation buildSeekerObservation(const MatchState&);

} // namespace hider_seeker