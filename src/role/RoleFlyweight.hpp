#pragma once

#include <string>

#include "Array.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "RoleEnum.hpp"
#include "TraitBits.hpp"
#include "TraitModifier.hpp"

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
    int attributes = 0;
    const char* name;
    TraitBits traitsSourced;
    Maybe<TraitModifier> biteTraitModifier;

    Pointer<const iActivator> activator;

    static const Array<RoleFlyweight, ROLE_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, RoleEnum& output);
};
