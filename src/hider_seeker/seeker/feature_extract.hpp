#pragma once

#include "feature_schema.hpp"
#include "../domain/state.hpp"

namespace hider_seeker {

SeekerObservation buildSeekerObservation(const MatchState&);

} // namespace hider_seeker