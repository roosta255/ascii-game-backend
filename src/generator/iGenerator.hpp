#pragma once

class Match;

class iGenerator {
public:
    virtual bool generate(int seed, Match&) const = 0;
};
