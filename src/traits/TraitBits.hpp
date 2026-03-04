#pragma once

#include "Bitstick.hpp"
#include <initializer_list>
#include "TraitEnum.hpp"

using TraitBits = Bitstick<(size_t)TRAIT_COUNT>;

TraitBits makeTraitBits(std::initializer_list<TraitEnum> setList);
