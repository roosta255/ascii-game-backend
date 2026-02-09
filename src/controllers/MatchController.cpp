#include "ActionFlyweight.hpp"
#include "AssociatedPriorityMinQueue.hpp"
#include "ChannelEnum.hpp"
#include "CharacterAction.hpp"
#include "CharacterDigest.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int3.hpp"
#include "ItemFlyweight.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "RoleFlyweight.hpp"

MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset) {}

// functions
bool MatchController::activate(const iActivator& activator, const Preactivation& preactivation) {
    setupLocations();

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    bool isSuccess = false;
    codeset.addFailure(!match.getPlayer(preactivation.playerId, codeset.error).access([&](Player& player) {
        codeset.addFailure(!match.getCharacter(preactivation.characterId, codeset.error).access([&](Character& character) {
            codeset.addFailure(!match.dungeon.getRoom(preactivation.roomId, codeset.error).access([&](Room& room) {
                CodeEnum error = CODE_UNSET;
                Activation activation{
                    .player = player,
                    .character = character,
                    .room = room,
                    .target = match.getCharacter(preactivation.targetCharacterId.orElse(-1), error),
                    .item = player.inventory.items.getPointer(preactivation.targetItemIndex.orElse(-1)),
                    .direction = preactivation.direction,
                    .match = match,
                    .codeset = codeset,
                    .time = preactivation.time,
                    .controller = *this,
                    .floorId = preactivation.floorId,
                    .isSkippingAnimations = preactivation.isSkippingAnimations,
                    .isSortingState = preactivation.isSortingState
                };
                codeset.addFailure(!(isSuccess = activator.activate(activation)));
            }));
        }));
    }));
    return isSuccess;
}

bool MatchController::addCharacterToFloor(const Character& source, int roomId, ChannelEnum channel, int& characterId) {
    setupLocations(false);

    // Validate room ID and get room
    auto room = match.dungeon.getRoom(roomId, codeset.error);
    if (room.isEmpty())
        return false;

    // Find an empty character slot
    bool isSuccess = false;
    codeset.addFailure(!match.allocateCharacter([&](Character& character){

        // Find an empty floor cell
        int floorId;
        if (codeset.addFailure(!findFreeFloor(roomId, channel, floorId)))
            return;
        // set character
        character = source;
        character.location = Location::makeFloor(roomId, channel, floorId);
        character.location.apply(character.characterId, match.dungeon.rooms, floors, doors);
        isSuccess = true;

    }), CODE_UNABLE_TO_FIND_FREE_CHARACTER_IN_DUNGEON);
    return isSuccess;
}

bool MatchController::assignCharacterToFloor(int characterId, int roomId, ChannelEnum channel, int floorId) {
    bool isSuccess = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
        codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
            int conflictingCharacterId;
            if (codeset.addFailure(isFloorOccupied(roomId, channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
                return;
            }

            // update location
            const auto newLocation = Location::makeFloor(roomId, channel, floorId);
            Location oldLocation;
            updateCharacterLocation(character, newLocation, oldLocation);
            isSuccess = true;
        }));
    }));
    return isSuccess;
}

bool MatchController::findCharacterPath(
    const std::string& playerId,
    int characterId,
    int loops,
    std::function<bool(const Match&)> destination,
    std::function<int(const Match&)> heuristic,
    std::function<void(const CharacterAction&, const Match&)> consumer
) {
    Match start = this->match;
    start.setPathfinding();
    auto frontier = AssociatedPriorityMinQueue<Match>();
    frontier.push(0, start);

    Map<Match, Match> came_from;
    Map<Match, int> cost_so_far;
    Map<Match, CharacterAction> action_from;

    cost_so_far.set(start, 0);
    Match goalMatch;
    bool isFound = false;

    while (true) {
        if (codeset.addFailure(frontier.isEmpty(), CODE_PATHFINDING_EXHAUSTED_ALL_POSSIBLITIES)) {
            return false;
        }
        if (codeset.addFailure(loops-- <= 0, CODE_PATHFINDING_FAILED_DUE_TO_INSUFFICIENT_LOOPS)) {
            return false;
        }
        // We capture the match by value to process, then find its stable pointer later
        frontier.pop([&](const Match& current) {
            if (destination(current)) {
                isFound = true;
                goalMatch = current; 
                loops = 0;
                return;
            }

            Match tempMatch = current;
            MatchController controller(tempMatch, codeset);
            controller.permuteCharacterActions(playerId, characterId, [&](const CharacterAction& action, const Match& permuted) {
                Match next = permuted;
                codeset.addFailure(!next.endTurn(playerId, codeset.error), CODE_PATHFINDING_FAILED_TO_FORWARD_TURN);
                next.setPathfinding();
                int new_cost = cost_so_far.getOrDefault(current, 0) + 1;

                if (!cost_so_far.containsKey(next) || new_cost < cost_so_far.getOrDefault(next, 0)) {
                    cost_so_far.set(next, new_cost);
                    int priority = new_cost + heuristic(next);
                    frontier.push(priority, next);
                    // These sets ensure 'next' exists as a key in our maps
                    came_from.set(next, current);
                    action_from.set(next, action);
                }
            });
        });

        if (isFound) {
            // Path Reconstruction using Pointers
            std::vector<const Match*> path_sequence;
            const Match* step = &goalMatch;

            while (step && !(*step == start)) {
                path_sequence.push_back(step);

                // Jump to the next parent pointer
                // came_from.get_ptr returns the pointer to the Match stored in the map
                came_from.getPointer(*step).access([&](Match& matchCamedFrom){
                    step = &matchCamedFrom;
                });
            }

            // Execute in chronological order
            for (auto it = path_sequence.rbegin(); it != path_sequence.rend(); ++it) {
                const Match* m = *it;
                action_from.accessConst(*m, [&](const CharacterAction& actionFrom){
                    consumer(actionFrom, *m);
                });
            }

            return true;
        }
    }
    return false;
}

bool MatchController::findFreeFloor(int roomId, ChannelEnum channel, int& output) {
    setupLocations();

    auto room = match.dungeon.getRoom(roomId, codeset.error);
    if (codeset.addFailure(room.isEmpty(), CODE_ROOM_NOT_WITHIN_DUNGEON))
        return false;

    bool isSuccess = false;
    room.accessConst([&](const Room& room){
        const auto floorSize = room.getFloorSize();
        const auto floorCount = floorSize[0] * floorSize[1];
        for (int floorId = 0; floorId < floorCount; floorId++) {
            int outCharacterId;
            if (!isFloorOccupied(roomId, channel, floorId, outCharacterId, match, floors)) {
                isSuccess = true;
                output = floorId;
                return;
            }
        }
    });

    return isSuccess;
}

bool MatchController::generate(int seed) {
    bool success = false;
    codeset.addFailure(!GeneratorFlyweight::getFlyweights().accessConst(match.generator, [&](const GeneratorFlyweight& flyweight){
        flyweight.generator.accessConst([&](const iGenerator& generator){

            // setup characterId
            match.accessAllCharacters([&](Character& character){
                codeset.addFailure(!match.containsCharacter(character, character.characterId), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_CHARACTER);
            });

            // setup roomId
            for (Room& room: match.dungeon.rooms) {
                codeset.addFailure(!match.dungeon.containsRoom(room, room.roomId), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_ROOM);
            }

            // generate match
            codeset.addFailure(!(success = generator.generate(seed, match, codeset)));
        });
    }), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_GENERATOR_FLYWEIGHT);
    return success;
}

const Map<int, Map<int2, int> >& MatchController::getDoors() {
    setupLocations(false);
    return doors;
}

const Map<int, Map<int2, int> >& MatchController::getFloors() {
    setupLocations(false);
    return floors;
}

bool MatchController::giveInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun) {
    return !codeset.addFailure(!inventory.giveItem(type, codeset.error, isDryrun));
}

bool MatchController::isCharacterActorValidation(const Character& character, const bool isCheckingCount) {
    return !codeset.addFailure(!character.isActor(codeset.error, isCheckingCount));
}

bool MatchController::isCharacterMoverValidation(const Character& character, const bool isCheckingCount) {
    return !codeset.addFailure(!character.isMovable(codeset.error, isCheckingCount));
}

bool MatchController::isCharacterKeyerValidation(const Character& character) {
    return !codeset.addFailure(!character.isKeyer(codeset.error));
}

bool MatchController::isCharacterWithinRoom(int characterId, int roomId, bool& result) {
    bool isSuccess = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
        isSuccess = character.location.accessRoomIds(match.dungeon.rooms, [&](const int& locationRoomId){
            if (roomId == locationRoomId) {
                result = true;
            }
        });
    }));
    return isSuccess;
}

bool MatchController::isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId, const Match& match, const Map<int, Map<int2, int> >& doors) {
    outCharacterId = -1;
    doors.accessConst(roomId, [&](const Map<int2, int>& mapping){
        outCharacterId = mapping.getOrDefault(int2{channel, dir.getIndex()}, -1);
    });
    return match.containsOffset(outCharacterId);
}

bool MatchController::isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId) {
    return isDoorOccupied(roomId, channel, dir, outCharacterId, match, doors);
}

bool MatchController::isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId, const Match& match, const Map<int, Map<int2, int> >& floors) {
    outCharacterId = -1;
    floors.accessConst(roomId, [&](const Map<int2, int>& mapping){
        outCharacterId = mapping.getOrDefault(int2{channel, floorId}, -1);
    });
    return match.containsOffset(outCharacterId);
}

bool MatchController::isFloorOccupied(int roomId, ChannelEnum channel, int floorId,int& outCharacterId) {
    return isFloorOccupied(roomId, channel, floorId, outCharacterId, match, floors);
}

bool MatchController::permuteCharacterActions(const std::string& playerId, int mainCharacterId, std::function<void(const CharacterAction&, const Match&)> consumer) {
    bool result = false;
    codeset.addFailure(!match.getCharacter(mainCharacterId, codeset.error).access([&](Character& character){
        
        character.location.accessRoomIds(match.dungeon.rooms, [&](const int& roomId){
            const auto isDoored = character.location.type == LOCATION_DOOR || character.location.type == LOCATION_DOOR;

            // helper function with switch
            const auto applyAction = [&](bool isDebugging, const CharacterAction& action){
                Match newMatch = this->match;
                Codeset nullCodes;
                MatchController newController(newMatch, isDebugging ? codeset : nullCodes);

                ActionFlyweight::getFlyweights().accessConst(action.type, [&](const ActionFlyweight& flyweight){
                    flyweight.activator.accessConst([&](const iActivator& activator){
                        Preactivation preactivation{
                            .playerId = playerId,
                            .characterId = action.characterId,
                            .roomId = action.roomId,
                            .targetCharacterId = action.targetCharacterId,
                            .targetItemIndex = action.targetItemIndex,
                            .direction = action.direction,
                            .floorId = action.floorId,
                            .isSkippingAnimations = true,
                            .isSortingState = true
                        };
                        if (!newController.codeset.addFailure(!newController.activate(activator, preactivation), CODE_PATHFINDING_ACTIVATION_FAILED)){
                            consumer(action, newMatch); // success
                        }
                    });
                });
            };

            CharacterDigest digest;
            if (codeset.addFailure(!character.getDigest(codeset.error, digest))) {
                return;
            }

            // out of actions/moves may be good time to end turn
            if (digest.actionsRemaining == 0 || digest.movesRemaining == 0) {
                applyAction(false, CharacterAction{ .type = ACTION_END_TURN, .characterId = mainCharacterId, .roomId = roomId });
            }

            // movements
            if (digest.movesRemaining.orElse(0) > 0) {
                // if move to a wall, unless we're on that specific wall
                for (const Cardinal dir: Cardinal::getAllCardinals()) {
                    if (!isDoored || (isDoored && Cardinal(character.location.data) != dir)) {
                        applyAction(false, CharacterAction{ .type = ACTION_MOVE_TO_DOOR, .characterId = mainCharacterId, .roomId = roomId, .direction = Maybe<Cardinal>(dir) });
                    }
                }

                // move to a floor, unless already on any floor
                int freeFloorId = -1;
                if (character.location.type != LOCATION_FLOOR && findFreeFloor(roomId, character.location.channel, freeFloorId)) {
                    applyAction(false, CharacterAction{ .type = ACTION_MOVE_TO_FLOOR, .characterId = mainCharacterId, .roomId = roomId, .floorId = freeFloorId });
                }
            }

            const auto processTargetCharacter = [&](CharacterAction action, const int targetCharacterId){
                if (targetCharacterId == mainCharacterId) {
                    return;
                }
                action.targetCharacterId = targetCharacterId;
                auto targetCharacterPtr = match.getCharacter(targetCharacterId, codeset.error);
                targetCharacterPtr.accessConst([&](const Character& targetCharacter){
                    switch (targetCharacter.role) {
                        case ROLE_TOGGLER:
                            action.characterId = targetCharacterId;
                            action.targetCharacterId = mainCharacterId;
                            return applyAction(false, action);
                    }
                });
            };

            // character actions
            if (digest.actionsRemaining.orElse(0) > 0) {
                
                for (const auto it: floors.getOrDefault(roomId, Map<int2, int>())) { // floors
                    const auto key = it.first;
                    processTargetCharacter(CharacterAction{ .type = ACTION_ACTIVATE_CHARACTER, .characterId = mainCharacterId, .roomId = roomId }, it.second);
                }

                for (const auto it: doors.getOrDefault(roomId, Map<int2, int>())) { // doors
                    const auto key = it.first;
                    processTargetCharacter(CharacterAction{ .type = ACTION_ACTIVATE_CHARACTER, .characterId = mainCharacterId, .roomId = roomId }, it.second);
                }

                // activations
                // ACTION_DECL(ACTIVATE_INVENTORY_ITEM)
            }

            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).accessConst([&](const Room& room){
                for (const Cardinal dir: Cardinal::getAllCardinals()) {
                    // check the doors for locks
                    const auto& wall = room.getWall(dir);
                    DoorFlyweight::getFlyweights().accessConst(wall.door, [&](const DoorFlyweight& flyweight){
                        bool isDebugging = false;
                        switch(wall.door){
                            case DOOR_TIME_GATE_AWAKENED: isDebugging = true;
                        }
                        if (flyweight.isDoorActionable) {
                            applyAction(isDebugging, CharacterAction{ .type = ACTION_ACTIVATE_DOOR, .characterId = mainCharacterId, .roomId = roomId, .direction = dir });
                        }
                        if (flyweight.isLockActionable) {
                            applyAction(false, CharacterAction{ .type = ACTION_ACTIVATE_LOCK, .characterId = mainCharacterId, .roomId = roomId, .direction = dir });
                        }
                    });
                }
            }), CODE_PATHFINDING_FAILED_TO_ACCESS_PATHFINDERS_ROOM);
        });

        result = true;
    }));
    return result;
}

void MatchController::setupLocations(bool isForced) {
    if (isLocationsSetup && !isForced) {
        return;
    }

    floors.clear();
    doors.clear();

    match.accessUsedCharacters([&](const Character& character){
        int characterId = -1;
        match.containsCharacter(character, characterId);
        character.location.apply(characterId, match.dungeon.rooms, floors, doors);
    });

    isLocationsSetup = true;
}

void MatchController::sortFloorCharacters(int roomId) {
    
}

bool MatchController::takeCharacterAction(Character& character) {
    return !codeset.addFailure(!character.takeAction(codeset.error));
}

bool MatchController::takeCharacterMove(Character& character) {
    return !codeset.addFailure(!character.takeMove(codeset.error));
}

bool MatchController::takeInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun) {
    return !codeset.addFailure(!inventory.takeItem(type, codeset.error, isDryrun));
}

bool MatchController::updateCharacterLocation(Character& character, const Location& newLocation, Location& oldLocation) {
    setupLocations(false);

    int characterId;
    if (codeset.addFailure(!match.containsCharacter(character, characterId))) {
        return false;
    }

    oldLocation = character.location;
    character.location = newLocation;

    oldLocation.apply(-1, match.dungeon.rooms, floors, doors);
    character.location.apply(characterId, match.dungeon.rooms, floors, doors);

    return true;
}

bool MatchController::validateCharacterWithinRoom(int characterId, int roomId) {
    bool isSubjectWithinRoom = false;
    if (codeset.addFailure(!isCharacterWithinRoom(characterId, roomId, isSubjectWithinRoom), CODE_FAILED_TO_CHECK_CHARACTER_WITHIN_ROOM)) {
        return false;
    }
    return !codeset.addFailure(!isSubjectWithinRoom, CODE_CHARACTER_NOT_IN_ROOM);
}

bool MatchController::validateDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir) {
    int outCharacterId = -1;
    if (codeset.addFailure(isDoorOccupied(roomId, channel, dir, outCharacterId), CODE_DOORWAY_OCCUPIED_BY_CHARACTER)) {
        codeset.setTable(CODE_OCCUPIED_CHARACTER_ID, outCharacterId);
        return false;
    }
    return true;
}

bool MatchController::validateSharedDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir) {
    int outCharacterId = -1;
    if (codeset.addFailure(isDoorOccupied(roomId, channel, dir, outCharacterId), CODE_DOORWAY_OCCUPIED_BY_CHARACTER)) {
        codeset.setTable(CODE_DOORWAY_OCCUPIED_CHARACTER_ID, outCharacterId);
        return false;
    }
    bool isValid = false;
    match.dungeon.rooms.accessConst(roomId, [&](const Room& room){
        const auto adjacentRoomId = room.getWall(dir).adjacent;
        match.dungeon.rooms.accessConst(adjacentRoomId, [&](const Room& adjacentRoom){
            bool isSharedDoorway = false;
            if (codeset.addFailure(!adjacentRoom.getWall(dir.getFlip()).readIsSharedDoorway(codeset.error, isSharedDoorway), CODE_SHARED_DOORWAY_CALLED_ON_DOOR_NOT_FLAGGED_AS_SHARED)){
                return;
            }
            if (codeset.addFailure(isDoorOccupied(adjacentRoomId, channel, dir.getFlip(), outCharacterId), CODE_SHARED_DOORWAY_OCCUPIED_BY_CHARACTER)) {
                codeset.setTable(CODE_SHARED_DOORWAY_OCCUPIED_CHARACTER_ID, outCharacterId);
                return;
            }
            isValid = true;
        });
    });
    return isValid;
}
