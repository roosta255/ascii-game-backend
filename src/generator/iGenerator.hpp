#pragma once

#include <functional>

class Codeset;
class Match;

class iGenerator {
public:
    virtual bool generate(int seed, Match&, Codeset& codeset) const = 0;
};
