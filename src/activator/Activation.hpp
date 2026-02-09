#pragma once

#include "Cardinal.hpp"
#include "Maybe.hpp"
#include "CodeEnum.hpp"
#include "Item.hpp"
#include "Pointer.hpp"
#include "Timestamp.hpp"

class Player;
class Character;
class Room;
class Match;
class MatchController;
class Codeset;

struct Activation {
    Player& player;
    Character& character;
    Room& room;
    Pointer<Character> target;
    Pointer<Item> item;
    Maybe<Cardinal> direction;
    Match& match;
    Codeset& codeset;
    Timestamp time;
    MatchController& controller;
    Maybe<int> floorId;
    bool isSkippingAnimations = false;
    bool isSortingState = false;
}; 