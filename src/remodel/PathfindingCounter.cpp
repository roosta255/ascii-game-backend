#include "ActionEnum.hpp"
#include "Array.hpp"
#include "Bitstick.hpp"
#include "Cardinal.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include <functional>
#include "generateRandomArray.hpp"
#include "PathfindingCounter.hpp"

bool PathfindingCounter::loopDoorways(std::function<bool(const int&, const Cardinal&)> consumer, const int seed) const {
    static const auto DOOR_IDS = generateRandomArray<4>(356376);
    static const auto ROOM_IDS = generateRandomArray<DUNGEON_ROOM_COUNT>(2364332);
    bool isDone = false;
    for (int d = 0; d < 4 && !isDone; d++) {
        const auto dir = Cardinal(DOOR_IDS.getWindow(d + seed));
        doorwayMoveCounter.doors.accessConst(dir.getIndex(), [&](const Bitstick<DUNGEON_ROOM_COUNT>& rooms){
            for (int i = 0; i < DUNGEON_ROOM_COUNT && !isDone; i++) {
                const auto index = ROOM_IDS.getWindow(i + seed);
                if (rooms[index]) {
                    isDone = consumer(index, dir);
                }
            }
        });
    }
    return isDone;
}

bool PathfindingCounter::loopRooms(std::function<bool(const int&)> consumer, const int seed) const {
    static const auto ROOM_IDS = generateRandomArray<DUNGEON_ROOM_COUNT>(1238842);
    bool isDone = false;
    for (int i = 0; i < DUNGEON_ROOM_COUNT && !isDone; i++) {
        const auto index = ROOM_IDS.getWindow(i + seed);
        if (doorwayMoveCounter.rooms[index]) {
            isDone = consumer(index);
        }
    }
    return isDone;
}
