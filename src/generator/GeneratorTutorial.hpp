#pragma once

#include "Cardinal.hpp"
#include "iGenerator.hpp"
#include "int4.hpp"
#include "Rack.hpp"
#include "Room.hpp"
// provides entrance
// provides neighbors

class GeneratorTutorial : public iGenerator {
public:
    bool generate(int seed, Match&) const override;
};
