#pragma once

#include "Cell.hpp"
#include "CodeEnum.hpp"
#include <functional>

class DoorFlyweight;
class Player;
class Character;
class JsonParameters;
class Room;
class Match;
class Cardinal;

struct Wall {
    int door = 0;
    Cell cell;

    bool isWalkable(CodeEnum& error) const;
    CodeEnum activate(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match);

private:
    // the last thing i want to do is to have a function that takes a function as a parameter
    bool accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)>) const;

};