#pragma once

#include "AnimationType.hpp"
#include "Bitstick.hpp"
#include <initializer_list>

using AnimationTypes = Bitstick<(size_t)ANIMATION_TYPE_COUNT>;

AnimationTypes makeAnimationTypes(std::initializer_list<AnimationType> setList);
