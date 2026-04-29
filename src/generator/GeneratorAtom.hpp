#pragma once

#include "iGenerator.hpp"

class GeneratorAtom : public iGenerator {
public:
    constexpr static int CANONICAL_SEED = 42;
    constexpr static int START_ROOM     = 0;  // room index of player start in LAYOUT_2D_8x8

    bool generate(int seed, Match&, Codeset& codeset) const override;
};
