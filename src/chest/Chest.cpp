
#include "Chest.hpp"
#include "Dungeon.hpp"

Pointer<Character> Chest::getContainerCharacter(Dungeon& dungeon) {
    return dungeon.characters.getPointer(containerCharacterId);
}

Pointer<Character> Chest::getCritterCharacter(Dungeon& dungeon) {
    return dungeon.characters.getPointer(critterCharacterId);
}

std::ostream& operator<<(std::ostream& os, const Chest& rhs) {
    return os;
}
