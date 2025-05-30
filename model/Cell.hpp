#pragma once

#include "Pointer.hpp"

namespace Json {
    class Value;
}

class Match;
class Character;

struct Cell {
    int offset = 0;

    bool setCharacter(const Match&, const Character&);
    Pointer<Character> getCharacter(Match&);

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};
