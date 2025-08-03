#pragma once

#include "Pointer.hpp"

class JsonParameters;
class Match;
class Character;

struct Cell {
    int offset = 0;
    bool setCharacter(const Match&, const Character&);
    Pointer<Character> getCharacter(Match&);
};
