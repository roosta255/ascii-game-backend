#pragma once

#include "Bitstick.hpp"
#include "DamageAttrEnum.hpp"

using DamageAttrBits = Bitstick<(size_t)DAMAGE_ATTR_COUNT>;
DamageAttrBits makeDamageAttrBits(std::initializer_list<DamageAttrEnum> attrs);
