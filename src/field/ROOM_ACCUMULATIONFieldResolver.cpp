#include "ROOM_ACCUMULATIONFieldResolver.hpp"
#include <algorithm>
#include "DUNGEON_ROOM_COUNT.hpp"
#include "FieldFlyweight.hpp"
#include "FieldState.hpp"
#include "iFieldSourceProvider.hpp"

void ROOM_ACCUMULATIONFieldResolver::resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const {
    for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
        state.values[roomId] = std::clamp(source.getRoomBaseValue(match, roomId), flyweight.minValue, flyweight.maxValue);
    }
}
