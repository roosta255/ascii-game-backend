#include "DamageTypeBits.hpp"

DamageTypeBits makeDamageTypeBits(std::initializer_list<DamageTypeEnum> types) {
    DamageTypeBits bits;
    for (const auto& type : types) {
        bits.setIndexOn(type);
    }
    return bits;
}
