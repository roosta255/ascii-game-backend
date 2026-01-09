#pragma once

#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "Item.hpp"
#include "Pointer.hpp"

class Player;
class Character;
class Room;
class Match;

struct Activation {
    Player& player;
    Character& character;
    Room& room;
    Pointer<Character> target;
    Pointer<Item> item;
    Cardinal direction;
    Match& match;

    Activation(
        Player& player,
        Character& character,
        Room& room,
        Pointer<Character> target,
        Cardinal direction,
        Match& match
    ) : player(player),
        character(character),
        room(room),
        target(target),
        direction(direction),
        match(match)
    {}

    Activation(
        Player& player,
        Character& character,
        Room& room,
        Item& item,
        Cardinal direction,
        Match& match
    ) : player(player),
        character(character),
        room(room),
        item(item),
        direction(direction),
        match(match)
    {}
}; 