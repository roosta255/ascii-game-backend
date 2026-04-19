#pragma once

#include "AnimationType.hpp"
#include "Bitstick.hpp"
#include <initializer_list>

using AnimationTypes = Bitstick<(size_t)ANIMATION_TYPE_TOTAL_COUNT>;

AnimationTypes makeAnimationTypes(std::initializer_list<AnimationType> setList);

// Reads the semantic-range bits of a types bitarray.
// Fills out[0..count-1] with up to 2 set AnimationSemantic values.
// Returns the number of semantics found.
int get_animation_semantics(const AnimationTypes& types, AnimationSemantic out[2]);
