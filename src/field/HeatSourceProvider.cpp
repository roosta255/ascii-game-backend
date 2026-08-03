#include "HeatSourceProvider.hpp"
#include "CodeEnum.hpp"
#include "Dungeon.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "TraitEnum.hpp"

namespace {
    constexpr int16_t SOURCE_STRENGTH = 100;
}

int16_t HeatSourceProvider::getRoomBaseValue(const Match& match, int roomId) const {
    bool isSource = false;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    match.dungeon.getRoom(roomId, error).accessConst([&](const Room& room) {
        RoomFlyweight::getFlyweights().accessConst(room.type, [&](const RoomFlyweight& flyweight) {
            if (flyweight.roomSourceAttributes[TRAIT_HEAT_SOURCE].orElse(false)) {
                isSource = true;
            }
        });
    });

    if (!isSource) {
        match.accessUsedCharacters([&](const Character& character) {
            if (character.location.roomId == roomId && character.traitsAfflicted[TRAIT_ENFLAMED].orElse(false)) {
                isSource = true;
            }
        });
    }

    return isSource ? SOURCE_STRENGTH : 0;
}
