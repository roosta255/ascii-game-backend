#pragma once

#include "Array.hpp"

class Match;

struct MatchApiParameters {
public:
    int mask = ~0x0;
    const Match& match;

    constexpr inline bool isHidden(const int target) const
    {
        return false;
        // this indicates the problem is with the model.visibility
        // return (0xffffffff & target) == 0 ? true : false;
        // return (this->mask & target) == 0 ? true : false;
    }
};

static_assert(0xffffffff == ~0x0, "");