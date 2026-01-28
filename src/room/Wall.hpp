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

    bool isWalkable(CodeEnum& error) const;
    bool activateDoor(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& error, MatchController& controller, Timestamp time = Timestamp());
    bool activateLock(Player& player, Character& character, Room& room, const Cardinal& direction, Match& match, Codeset& error, MatchController& controller, Timestamp time = Timestamp());

private:
    // the last thing i want to do is to have a function that takes a function as a parameter
    bool accessDoor(CodeEnum& error, std::function<void(const DoorFlyweight&)>) const;

};