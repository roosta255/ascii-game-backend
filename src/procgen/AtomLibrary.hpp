#pragma once

#include "Atom.hpp"
#include "ArchitectState.hpp"
#include <vector>

class AtomLibrary {
public:
    static const Atom* getAll(int& outCount);
    static std::vector<const Atom*> getCompatible(const ArchitectState& state);
};
