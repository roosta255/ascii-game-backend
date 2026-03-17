
#include "Chest.hpp"
#include "Dungeon.hpp"

Pointer<Character> Chest::getContainerCharacter(Dungeon& dungeon) {
    return dungeon.characters.getPointer(containerCharacterId);
}

std::ostream& operator<<(std::ostream& os, const Chest& rhs) {
    return os;
}
