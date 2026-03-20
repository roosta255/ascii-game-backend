#include "DamageAttrBits.hpp"

DamageAttrBits makeDamageAttrBits(std::initializer_list<DamageAttrEnum> attrs) {
    DamageAttrBits bits;
    for (const auto& attr : attrs) { bits.setIndexOn(attr); }
    return bits;
}
