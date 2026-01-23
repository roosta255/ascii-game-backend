#pragma once

#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "Item.hpp"
#include "Pointer.hpp"
#include "Timestamp.hpp"

class Player;
class Character;
class Room;
class Match;
class Codeset;

struct Activation {
    Player& player;
    Character& character;
    Room& room;
    Pointer<Character> target;
    Pointer<Item> item;
    Cardinal direction;
    Match& match;
    Codeset& codeset;
    Timestamp time;

    Activation(
        Player& player,
        Character& character,
        Room& room,
        Pointer<Character> target,
        Cardinal direction,
        Match& match,
        Codeset& codeset,
        Timestamp time = Timestamp()
    );

    Activation(
        Player& player,
        Character& character,
        Room& room,
        Item& item,
        Cardinal direction,
        Match& match,
        Codeset& codeset,
        Timestamp time = Timestamp()
    );
}; 