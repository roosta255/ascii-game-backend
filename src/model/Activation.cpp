#include "Activation.hpp"
#include "Match.hpp"
/*
int Activation::getRoomId()const{
    int roomId = -1;
    match.dungeon.containsRoom(this->room, roomId);
    return roomId;
}
*/
Activation::Activation(
    Player& player,
    Character& character,
    Room& room,
    Pointer<Character> target,
    Cardinal direction,
    Match& match,
    Codeset& codeset,
    Timestamp time
) : player(player),
    character(character),
    room(room),
    target(target),
    direction(direction),
    match(match),
    codeset(codeset),
    time(time)
{}

Activation::Activation(
    Player& player,
    Character& character,
    Room& room,
    Item& item,
    Cardinal direction,
    Match& match,
    Codeset& codeset,
    Timestamp time
) : player(player),
    character(character),
    room(room),
    item(item),
    direction(direction),
    match(match),
    codeset(codeset),
    time(time)
{}