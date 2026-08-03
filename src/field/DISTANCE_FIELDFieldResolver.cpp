#include "DISTANCE_FIELDFieldResolver.hpp"
#include <algorithm>
#include <queue>
#include "Bitstick.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "Dungeon.hpp"
#include "FieldFlyweight.hpp"
#include "FieldState.hpp"
#include "iFieldSourceProvider.hpp"
#include "Match.hpp"
#include "Room.hpp"

namespace {
    constexpr int16_t DECAY_PER_ROOM = 25;
}

void DISTANCE_FIELDFieldResolver::resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const {
    Bitstick<(size_t)DUNGEON_ROOM_COUNT> visited;
    std::queue<int> frontier;

    for (int roomId = 0; roomId < DUNGEON_ROOM_COUNT; roomId++) {
        const int16_t base = source.getRoomBaseValue(match, roomId);
        if (base <= 0) continue;

        state.values[roomId] = std::clamp(base, flyweight.minValue, flyweight.maxValue);
        visited.setIndexOn(roomId);
        frontier.push(roomId);
    }

    while (!frontier.empty()) {
        const int roomId = frontier.front();
        frontier.pop();

        const int16_t nextValue = (int16_t)(state.values[roomId] - DECAY_PER_ROOM);
        if (nextValue <= 0) continue;

        const auto visitNeighbor = [&](int neighborId) {
            if (visited[neighborId].orElse(true)) return;
            state.values[neighborId] = std::clamp(nextValue, flyweight.minValue, flyweight.maxValue);
            visited.setIndexOn(neighborId);
            frontier.push(neighborId);
        };

        match.dungeon.rooms.accessConst(roomId, [&](const Room& room) {
            // Room::roomId is never populated to the room's array index (see Room::above/below
            // instead), so neighbor identity comes from the source room's own fields / the
            // accessWallNeighbor callback's explicit id, never from a neighbor's own roomId member.
            match.dungeon.accessCeilingNeighbor(room, [&](const Room&) { visitNeighbor(room.above); });
            match.dungeon.accessFloorNeighbor(room, [&](const Room&) { visitNeighbor(room.below); });
            for (Cardinal dir : Cardinal::getAllCardinals()) {
                match.dungeon.accessWallNeighbor(room, dir, [&](const Wall&, const Room&, int neighborId) { visitNeighbor(neighborId); });
            }
        });
    }
}
