#pragma once

#include <string>

#include "Array.hpp"
#include "Pointer.hpp"
#include "RoleEnum.hpp"

class Character;
class iActivator;
class Match;
class Player;
class Room;

struct RoleFlyweight {
    int health = 0;
    int feats = 0;
    int actions = 0;
    int moves = 0;
    bool isActionable = false;
    bool isMovable = false;
    bool isActor = false;
    bool isKeyer = false;  // Whether this role can use keys
    bool isObject = false;  // Whether this role can be used by other characters
    int attributes = 0;
    const char* name;
    bool foggy = false;

    Pointer<const iActivator> activator;

    static const Array<RoleFlyweight, ROLE_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, int& output);
};
