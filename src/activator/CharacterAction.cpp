
#include "CharacterAction.hpp"

std::ostream& operator<<(std::ostream& os, const CharacterAction& rhs) {
    os << "CharacterAction{.type=" << action_to_text(rhs.type) << ", .characterId=" << rhs.characterId << ", .roomId=" << rhs.roomId;
    rhs.targetCharacterId.accessConst([&](const auto& field){
        os << ", .targetCharacterId=" << field;
    });
    rhs.targetItemIndex.accessConst([&](const auto& field){
        os << ", .targetItemIndex=" << field;
    });
    rhs.direction.accessConst([&](const auto& field){
        os << ", .direction=" << field.getIndex();
    });
    rhs.floorId.accessConst([&](const auto& field){
        os << ", .floorId=" << field;
    });
    return os << " }";
}
