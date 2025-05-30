#pragma once

#include "Match.hpp"

class iGenerator {
public:
    virtual bool generate(int seed, Match&) const = 0;
    bool generate(const Match&, int seed, Match&) const;
};
