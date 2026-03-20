#pragma once

#include "Bitstick.hpp"
#include "DamageTypeEnum.hpp"

using DamageTypeBits = Bitstick<(size_t)DAMAGE_TYPE_COUNT>;

DamageTypeBits makeDamageTypeBits(std::initializer_list<DamageTypeEnum> types);
