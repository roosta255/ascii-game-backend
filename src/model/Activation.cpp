#include "Activation.hpp"
#include "Match.hpp"

int Activation::getRoomId()const{
    int roomId = -1;
    match.dungeon.containsRoom(this->room, roomId);
    return roomId;
}