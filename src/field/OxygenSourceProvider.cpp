#include "OxygenSourceProvider.hpp"
#include "CodeEnum.hpp"
#include "Dungeon.hpp"
#include "Match.hpp"
#include "Room.hpp"
#include "RoomFlyweight.hpp"
#include "TraitEnum.hpp"

namespace {
    constexpr int16_t AMBIENT_OXYGEN = 60;
    constexpr int16_t VENT_BONUS = 40;
    constexpr int16_t ENFLAMED_CONSUMPTION = 20;
}

int16_t OxygenSourceProvider::getRoomBaseValue(const Match& match, int roomId) const {
    int16_t value = AMBIENT_OXYGEN;

    CodeEnum error = CODE_UNKNOWN_ERROR;
    match.dungeon.getRoom(roomId, error).accessConst([&](const Room& room) {
        RoomFlyweight::getFlyweights().accessConst(room.type, [&](const RoomFlyweight& flyweight) {
            if (flyweight.roomSourceAttributes[TRAIT_OXYGEN_VENT].orElse(false)) {
                value += VENT_BONUS;
            }
        });
    });

    match.accessUsedCharacters([&](const Character& character) {
        if (character.location.roomId == roomId && character.traitsAfflicted[TRAIT_ENFLAMED].orElse(false)) {
            value -= ENFLAMED_CONSUMPTION;
        }
    });

    return value;
}
