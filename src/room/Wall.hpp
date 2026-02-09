#pragma once

#include "CodeEnum.hpp"
#include <functional>
#include "Timestamp.hpp"

class Codeset;
class DoorFlyweight;
class Player;
class Character;
class JsonParameters;
class Room;
class Match;
class MatchController;
class Cardinal;

struct Wall {
    int door = 0;
    int adjacent = -1;

    // functions
    bool accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)>) const;
    bool isWalkable(CodeEnum& error) const;
    bool readIsSharedDoorway(CodeEnum& error, bool& isSharedDoorway) const;

};

// operators

std::ostream& operator<<(std::ostream& os, const Wall& rhs);
