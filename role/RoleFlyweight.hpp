#pragma once

#include <string>

#include "Array.hpp"
#include "RoleEnum.hpp"

class Character;

struct RoleFlyweight {
    int health = 0;
    int feats = 0;
    int actions = 0;
    int moves = 0;
    bool isActionable = false;
    bool isMovable = false;
    int attributes = 0;
    const char* name;
    bool foggy = false;

    static const Array<RoleFlyweight, ROLE_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
