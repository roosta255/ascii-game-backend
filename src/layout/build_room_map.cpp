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

    bool isFailure = false;
    rooms.getPointer(0).access([&](Room& start){

        q.push({int4{0, 0, 0, 0}, Pointer<Room>(start)});
        discovered[int4{0, 0, 0, 0}] = Pointer<Room>(start);

        constexpr int MAX_ITERATIONS = 64;
        int i = 0;
        while (!q.empty()) {
            if (i++ >= MAX_ITERATIONS) {
                isFailure = true;
                return;
            }

            auto [pos, ptr] = q.front(); q.pop();

            ptr.accessConst([&](Room& room){
                const auto queueRoomIfUndiscovered = [&](Room& neighbor, const int4& newPos){
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
                };
                for (const auto dir: Cardinal::getAllCardinals()) {
                    layout.getWallNeighbor(rooms, room, dir).access([&](Room& neighbor){
                        queueRoomIfUndiscovered(neighbor, pos + to_int4(dir.getRectOffset()));
                    });
                }

                const auto queueNextRoomByDepth = [&](int delta){
                    int next = -1;
                    if (layout.getDepthDelta(rooms, room, delta, next)) {
                        rooms.access(next, [&](Room& neighbor) {
                            queueRoomIfUndiscovered(neighbor, pos + int4{0,0,delta,0});
                        });
                    }
                };

                queueNextRoomByDepth(-1);
                queueNextRoomByDepth(1);

                const auto queueNextRoomByTime = [&](int delta){
                    int next = -1;
                    if (layout.getTimeDelta(rooms, room, delta, next)) {
                        rooms.access(next, [&](Room& neighbor) {
                            queueRoomIfUndiscovered(neighbor, pos + int4{0,0,0,delta});
                        });
                    }
                };

                queueNextRoomByTime(-1);
                queueNextRoomByTime(1);
            });
        }

        for (auto& [pos, ptr] : discovered) {
            const auto shiftedPos = pos - min;
            shifted[shiftedPos] = ptr;
        }
    });
    return isFailure ? int4{1,1,1,1} : max - min + int4{1,1,1,1};
}
