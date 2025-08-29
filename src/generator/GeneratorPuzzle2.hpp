#pragma once

#include "Cardinal.hpp"
#include "iGenerator.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class GeneratorPuzzle2 : public iGenerator {
public:
    bool generate(int seed, Match&) const override;
};
