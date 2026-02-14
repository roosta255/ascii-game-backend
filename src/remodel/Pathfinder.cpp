#include <cmath>
#include "ActionFlyweight.hpp"
#include "CharacterAction.hpp"
#include "Codeset.hpp"
#include "DoorEnum.hpp"
#include "HASH_MACRO_DECL.hpp"
#include "LRUMap.hpp"
#include "Map.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Pathfinder.hpp"
#include "PathfindingCounter.hpp"
#include <tuple>

struct CacheKey {
    std::string playerId;
    int characterId;
    int bossRoomId;
    Match match; // 16Kb object

    // Equality operator is required for the hash map
    bool operator==(const CacheKey& other) const {
        return playerId == other.playerId &&
               characterId == other.characterId &&
               bossRoomId == other.bossRoomId &&
               match == other.match; // Ensure Match has operator==
    }
};

namespace std {
    template <>
    struct hash<CacheKey> {
        std::size_t operator()(const CacheKey& k) const {
            // Start with the Match hash you already wrote
            std::size_t h = std::hash<Match>{}(k.match);

            // Helper to mix in the other fields
            auto hash_combine = [](std::size_t& seed, std::size_t v) {
                seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            hash_combine(h, std::hash<std::string>{}(k.playerId));
            hash_combine(h, std::hash<int>{}(k.characterId));
            hash_combine(h, std::hash<int>{}(k.bossRoomId));

            return h;
        }
    };
}

Pathfinder Pathfinder::build(const std::string& playerId, int pathCharacterId, const int bossRoomId, const Match& inputMatch, Codeset& codeset) {

    // 1. Static LRU Cache Instance
    static LRUMap<CacheKey, Pathfinder> cache(Pathfinder::MAX_CACHED);

    // 2. Create the lookup key
    CacheKey key{playerId, pathCharacterId, bossRoomId, inputMatch};

    // 3. Lookup in cache
    Pathfinder result;
    bool found = cache.access(key, [&](Pathfinder& cached) {
        result = cached; // Copy the 16Kb result out
    });

    codeset.addTable(CODE_PATHFINDER_A_STAR_CALL);
    if (found) {
        codeset.addTable(CODE_PATHFINDER_A_STAR_CACHE_HIT);
        return result;
    } else {
        codeset.addTable(CODE_PATHFINDER_A_STAR_CACHE_MISS);
    }

    const auto handleCounter = [&](PathfindingCounter& counter, const CharacterAction& action){
        const auto handleActionCounter = [&](PathfindingCounter::ActionCounter& actionCounter){
            actionCounter.count++;
            actionCounter.rooms.setIndexOn(action.roomId);
            action.direction.accessConst([&](const Cardinal& dir){
                actionCounter.doors.getPointer(dir.getIndex()).access([&](Bitstick<DUNGEON_ROOM_COUNT>& door){
                    door.setIndexOn(action.roomId);
                });
            });
        };

        const auto isDoorway = action.direction.map<bool>(
            [&](const Cardinal& dir){
                return inputMatch.dungeon.rooms.getPointer(action.roomId).map<bool>(
                    [&](const Room& room){
                        return room.getWall(dir).door == DOOR_DOORWAY;
                    }).orElse(false);
            }).orElse(false);

        // increment the action counter
        counter.actionCounter.getPointer(action.type).access([&](PathfindingCounter::ActionCounter& actionCounter){
            handleActionCounter(actionCounter);
        });

        // increment the action type counter
        ActionFlyweight::getFlyweights().accessConst(action.type, [&](const ActionFlyweight& flyweight){
            if (flyweight.type == ACTION_TYPE_MOVEMENT) {
                handleActionCounter(isDoorway ? counter.doorwayMoveCounter : counter.nonDoorwayMoveCounter);
            }

            counter.actionTypeCounter.getPointer(flyweight.type).access([&](PathfindingCounter::ActionCounter& actionCounter){
                handleActionCounter(actionCounter);
            });
        });
        
    };

    Match match = inputMatch;
    Pathfinder created;
    created.isSolved = MatchController(match, codeset).findCharacterPath(playerId, pathCharacterId, MAX_PATHFINDING_LOOPS,
        [&](const Match& candidate) {
            Match destination = candidate;
            MatchController controller(destination, codeset);
            bool isCharacterWithinRoom = false;
            if (controller.isCharacterWithinRoom(pathCharacterId, bossRoomId, isCharacterWithinRoom)) {
                return isCharacterWithinRoom;
            }
            return false;
        }, [&](const CharacterAction& action, const Match& heuristic) {
            created.counters.getPointer(PATHFIND_1_FRONTIER).access([&](PathfindingCounter& counter){
                handleCounter(counter, action);
            });
            return 0; // TODO: compute a real distance function to the boss room
        }, [&](const CharacterAction& action, const Match&) {
            created.counters.getPointer(PATHFIND_1_SOLUTION).access([&](PathfindingCounter& counter){
                handleCounter(counter, action);
            });
        }, false);

    if (created.isSolved) {
        codeset.addTable(CODE_PATHFINDER_A_STAR_FINDS_SOLUTION);
    } else {
        codeset.addTable(CODE_PATHFINDER_A_STAR_NO_SOLUTION_FOUND);
    }

    // 5. Store in cache and return
    cache.set(key, created);
    return created;
}

const PathfindingCounter& Pathfinder::getCounter() const {
    static PathfindingCounter fatalCounter;
    return counters.getOrDefault(isSolved ? Pathfinder::PATHFIND_1_SOLUTION : Pathfinder::PATHFIND_1_FRONTIER, fatalCounter);
}
