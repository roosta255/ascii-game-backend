
#include "TraitBits.hpp"

TraitBits makeTraitBits(std::initializer_list<TraitEnum> setList) {
    TraitBits bits;
    for (auto t : setList)
    {
        if (t == TRAIT_NIL) continue;
        bits.setIndexOn((size_t)t);
    }
    return bits;
}
