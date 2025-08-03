#include <unordered_map>
#include <queue>
#include <limits>
#include <array>
#include <tuple>

#include "Cardinal.hpp"
#include "build_room_map.hpp"
#include "iLayout.hpp"
#include "int4.hpp"
#include "Pointer.hpp"
#include "to_int4.hpp"

// rye <- ergot risk
// barley
// turnips
// parsnips
// nettles/sorrel
// lupins aka fava bean
// wild leek/wild garlic
// spelt?

int4 build_room_map(Array<Room, DUNGEON_ROOM_COUNT>& rooms, const iLayout& layout, std::unordered_map<int4, Pointer<Room>>& shifted) {

    std::unordered_map<int4, Pointer<Room>> discovered;
    std::queue<std::pair<int4, Pointer<Room>>> q;

    int4 min{0, 0, 0, 0};
    int4 max{0, 0, 0, 0};

    Room& start = layout.getEntrance(rooms);

    q.push({int4{0, 0, 0, 0}, Pointer<Room>(start)});
    discovered[int4{0, 0, 0, 0}] = Pointer<Room>(start);

    constexpr int MAX_ITERATIONS = 64;
    int i = 0;
    while (!q.empty()) {
        if (i++ >= MAX_ITERATIONS) {
            return int4{1,1,1,1};
        }

        auto [pos, ptr] = q.front(); q.pop();

        ptr.accessConst([&](Room& room){
            for (const auto dir: Cardinal::getAllCardinals()) {
                layout.getWallNeighbor(rooms, room, dir).accessConst([&](Room& neighbor){
                    const int4 newPos = pos + to_int4(dir.getRectOffset());

                    if (discovered.find(newPos) == discovered.end()) {
                        discovered[newPos] = neighbor;
                        q.push({newPos, neighbor});

                        min[0] = std::min(min[0], newPos[0]);
                        min[1] = std::min(min[1], newPos[1]);
                        min[2] = std::min(min[2], newPos[2]);
                        min[3] = std::min(min[3], newPos[3]);

                        max[0] = std::max(max[0], newPos[0]);
                        max[1] = std::max(max[1], newPos[1]);
                        max[2] = std::max(max[2], newPos[2]);
                        max[3] = std::max(max[3], newPos[3]);
                    }
                });
            }
        });
    }

    for (auto& [pos, ptr] : discovered) {
        const auto shiftedPos = pos - min;
        shifted[shiftedPos] = ptr;
    }

    return max - min + int4{1,1,1,1};
}
