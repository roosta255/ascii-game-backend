#pragma once

#include "Match.hpp"

class iGenerator {
public:
    virtual bool generate(int seed, Match&) const = 0;
};
