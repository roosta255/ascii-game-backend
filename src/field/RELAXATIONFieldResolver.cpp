#include "RELAXATIONFieldResolver.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Dungeon.hpp"
#include "FieldFlyweight.hpp"
#include "FieldState.hpp"
#include "iFieldSourceProvider.hpp"
#include "Match.hpp"
#include "Room.hpp"

void RELAXATIONFieldResolver::resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const {
    std::array<int16_t, DUNGEON_ROOM_COUNT> sourceValues{};
    for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
        sourceValues[roomId] = std::clamp(source.getRoomBaseValue(match, roomId), flyweight.minValue, flyweight.maxValue);
        state.values[roomId] = sourceValues[roomId];
    }

    constexpr size_t MAX_ITERATIONS = (size_t)DUNGEON_ROOM_COUNT * 4;
    bool changed = true;
    size_t iterations = 0;

    while (changed && iterations++ < MAX_ITERATIONS) {
        changed = false;
        std::array<int16_t, DUNGEON_ROOM_COUNT> next = state.values;

        for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
            int32_t sum = state.values[roomId];
            int32_t count = 1;

            const auto addNeighbor = [&](int neighborId) {
                if (neighborId < 0) return;
                sum += state.values[neighborId];
                count++;
            };

            match.dungeon.rooms.accessConst(roomId, [&](const Room& room) {
                // Room::roomId is never populated to the room's array index, so neighbor
                // identity comes from the source room's own fields / accessWallNeighbor's id.
                match.dungeon.accessCeilingNeighbor(room, [&](const Room&) { addNeighbor(room.above); });
                match.dungeon.accessFloorNeighbor(room, [&](const Room&) { addNeighbor(room.below); });
                for (Cardinal dir : Cardinal::getAllCardinals()) {
                    match.dungeon.accessWallNeighbor(room, dir, [&](const Wall&, const Room&, int neighborId) { addNeighbor(neighborId); });
                }
            });

            const int16_t equalized = (int16_t)(sum / count);
            const int16_t resolved = std::clamp<int16_t>(std::max(equalized, sourceValues[roomId]), flyweight.minValue, flyweight.maxValue);

            if (resolved != next[roomId]) {
                next[roomId] = resolved;
                changed = true;
            }
        }

        state.values = next;
    }
}
