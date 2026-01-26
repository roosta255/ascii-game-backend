#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int2.hpp"
#include "ItemFlyweight.hpp"
#include "Keyframe.hpp"
// #include "make_filename.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleFlyweight.hpp"

MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset) {
}

bool MatchController::activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time) {
    bool isSuccess = false;

    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                // NOTE: this error code isn't captured as a failure/success
                // this lets the iActivator needs to decide whether thats a problem
                auto target = match.getCharacter(targetId, codeset.error);
                isSuccess = codeset.addSuccessElseFailure(activateCharacter(player, character, room, target, time));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::activateCharacter(Player& player, Character& subject, Room& room, Pointer<Character> target, Timestamp time) {

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character can take an action
    if (codeset.addFailure(!subject.isActionable(codeset.error))) return false;

    int subjectOffset;
    if(codeset.addFailure(!match.containsCharacter(subject, subjectOffset), CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID)) {
        return false;
    }
    if (codeset.addFailure(!room.containsCharacter(subjectOffset), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isTargetValid = false;
    target.access([&](Character& target) {
        int targetOffset;
        if(codeset.addFailure(!match.containsCharacter(target, targetOffset), CODE_INACCESSIBLE_TARGET_CHARACTER_ID)) {
            return;
        }
        if (codeset.addFailure(!room.containsCharacter(targetOffset), CODE_TARGET_CHARACTER_NOT_IN_ROOM)) {
            return;
        }
        isTargetValid = true;
    });

    if (!isTargetValid) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!subject.accessRole(codeset.error, [&](const RoleFlyweight& flyweight) {
        if (codeset.addFailure(flyweight.activator.isEmpty(), CODE_MISSING_ACTIVATOR)) {
            return;
        } else {
            flyweight.activator.accessConst([&](const iActivator& activatorIntf){
                Activation activation(player, subject, room, target, Cardinal::north(), match, codeset, time);
                isSuccess = codeset.addSuccessElseFailureIfCodedSuccess(activatorIntf.activate(activation));
            });
        }
    }), CODE_INACCESSIBLE_ROLE_FLYWEIGHT);

    return isSuccess;
}

bool MatchController::activateDoor(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time) {
    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character is an actor
    if (codeset.addFailure(!character.isActor(codeset.error))) {
        return false;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);
    return codeset.addSuccessElseFailure(wall.activateDoor(player, character, room, direction, match, codeset, time));
}

bool MatchController::activateDoor(const std::string& playerId, int characterId, int roomId, int direction, Timestamp time) {
    bool isSuccess = false;
    // Get the player
    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        // Get the character
        codeset.setTable(CODE_ACTIVATOR_ID, characterId);
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            // Get the room
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                isSuccess = codeset.addSuccessElseFailure(activateDoor(player, character, room, Cardinal(direction), time));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::activateInventoryItem(const std::string& playerId, int characterId, int roomId, int itemId, Timestamp time) {
    bool isSuccess = false;

    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                codeset.addFailure(!player.inventory.accessItem(itemId, codeset.error, [&](Item& item){
                    isSuccess = codeset.addSuccessElseFailure(activateInventoryItem(player, character, room, item, time));
                }));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::activateInventoryItem(Player& player, Character& subject, Room& room, Item& item, Timestamp time) {

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character can take an action
    if (codeset.addFailure(!subject.isActor(codeset.error))) return false;

    int subjectOffset;
    if (codeset.addFailure(!match.containsCharacter(subject, subjectOffset), CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID)) {
        return false;
    }
    if (codeset.addFailure(!room.containsCharacter(subjectOffset), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!item.accessFlyweight([&](const ItemFlyweight& flyweight){
        flyweight.useActivator.accessConst([&](const iActivator& activatorIntf){
            Activation activation(player, subject, room, item, Cardinal::north(), match, codeset, time);
            isSuccess = codeset.addSuccessElseFailureIfCodedSuccess(activatorIntf.activate(activation));
        });
    }), CODE_INACCESSIBLE_INVENTORY_FLYWEIGHT);

    return isSuccess;
}

bool MatchController::activateLock(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time) {
    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character is an actor
    if (codeset.addFailure(!character.isActor(codeset.error))) {
        return false;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);
    return codeset.addSuccessElseFailure(wall.activateLock(player, character, room, direction, match, codeset, time));
}

bool MatchController::activateLock(const std::string& playerId, int characterId, int roomId, int direction, Timestamp time) {
    bool isSuccess = false;
    // Get the player
    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        // Get the character
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            // Get the room
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                isSuccess = codeset.addSuccessElseFailure(activateLock(player, character, room, Cardinal(direction), time));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::cleanupMovement(Character& character, Room& room, int& characterId, bool& wasFloored, int2& prevFloor, bool& wasWalled, Cardinal& prevWall) {
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check for movability
    if (codeset.addFailure(!character.isMovable(codeset.error, true))) return false;

    // Check if character is in room
    if (codeset.addFailure(!match.isCharacterInRoom(codeset.error, room, character), CODE_CLEANUP_CHARACTER_FAILED_ROOM_OCCUPANCY_TEST)) return false;

    if (codeset.addFailure(!match.containsCharacter(character, characterId), CODE_INACCESSIBLE_CHARACTER_ID)) {
        return false;
    }

    // Cleanup previous
    bool isCleanedUp = false;
    wasWalled = false;
    wasFloored = false;
    codeset.setTable(CODE_CLEANUP_ID, characterId);

    match.dungeon.findCharacter(
        room,
        characterId,
        [&](Cell& local, Cardinal wall, Room&, Cell& other) {
            local.offset = 0;
            other.offset = 0;
            isCleanedUp = true;
            wasWalled = true;
            prevWall = wall;
        },
        [&](int x, int y, Cell& cell) {
            cell.offset = 0;
            isCleanedUp = true;
            wasFloored = true;
            prevFloor = int2{x, y};
        }
    );
    return !codeset.addFailure(!isCleanedUp, CODE_MISSING_CHARACTER_CLEANUP);
}

bool MatchController::generate(int seed) {
    bool success = false;
    codeset.addFailure(!GeneratorFlyweight::getFlyweights().accessConst(match.generator, [&](const GeneratorFlyweight& flyweight){
        flyweight.generator.accessConst([&](const iGenerator& generator){
            success = codeset.addSuccessElseFailure(generator.generate(seed, match, codeset));
        });
    }), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_GENERATOR_FLYWEIGHT);
    return success;
}

bool MatchController::moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time) {
    bool result = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character){
        codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
            codeset.addFailure(!room.getCell(floorId, codeset.error).access([&](Cell& floor) {
                result = moveCharacterToFloor(room, character, floor, time);
            }));
        }));
    }));
    return result;
}

bool MatchController::moveCharacterToFloor(Room& room, Character& character, Cell& floor, Timestamp time) {

    int room0;
    if (codeset.addFailure(!match.dungeon.containsRoom(room, room0), CODE_ROOM_NOT_WITHIN_DUNGEON)) {
        return false;
    }

    // Check for occupied target cell
    if (codeset.addFailure(match.containsOffset(floor.offset), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
        return false;
    }

    // Cleanup previous
    int characterId;
    bool wasWalled, wasFloored;
    int2 prevFloor;
    Cardinal prevWall = Cardinal::north();
    if (codeset.addFailure(!cleanupMovement(character, room, characterId, wasFloored, prevFloor, wasWalled, prevWall))) return false;

    if (codeset.addFailure(!character.takeMove(codeset.error))) return false;

    // Set the character offset in the cell
    floor.offset = characterId;

    int2 destination{0,0};
    int index;
    CodeEnum error2;
    room.containsFloorCell(floor, error2, index, destination);

    // animate
    const Keyframe keyframe = wasFloored
        // floor -> floor
        ? Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, room0, prevFloor, destination)
        // wall -> floor
        : Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, room0, prevWall, destination);
    if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes), keyframe)) {
        // TODO: animation overflow
    }

    return codeset.addSuccessElseFailure(true);
}

bool MatchController::moveCharacterToWall(int roomId, int characterId, Cardinal direction, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character){
        codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
            isSuccess = codeset.addSuccessElseFailure(moveCharacterToWall(room, character, direction, time), CODE_MOVE_CHARACTER_TO_WALL_FAILED);
        }), CODE_INACCESSIBLE_ROOM_ID);
    }), CODE_INACCESSIBLE_CHARACTER_ID);

    return isSuccess;
}

bool MatchController::moveCharacterToWall(Room& room, Character& character, Cardinal direction, Timestamp time) {

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error), CODE_MATCH_NOT_STARTED)) return false;

    int room0;
    if (codeset.addFailure(!match.dungeon.containsRoom(room, room0), CODE_ROOM_NOT_WITHIN_DUNGEON)) {
        return false;
    }

    // Check for occupied target cell
    Wall& next = room.getWall(direction);
    if (codeset.addFailure(match.containsOffset(next.cell.offset), CODE_OCCUPIED_TARGET_WALL_CELL)) return false;

    if (codeset.addFailure(!next.isWalkable(codeset.error), CODE_DOOR_FAILED_WALKABILITY)) return false;


    int characterId;
    bool wasWalled, wasFloored;
    int2 prevFloor;
    Cardinal prevWall = Cardinal::north();
    if (codeset.addFailure(!cleanupMovement(character, room, characterId, wasFloored, prevFloor, wasWalled, prevWall), CODE_MOVE_CHARACTER_TO_WALL_FAILED_CLEANUP)) return false;

    bool isSuccess = false;
    codeset.addFailure(!match.dungeon.rooms.access(next.adjacent, [&](Room& room2){
        // this is where the actual move happens
        room2.getWall(direction.getFlip()).cell.offset = characterId;
        if (codeset.addSuccessElseFailure(character.takeMove(codeset.error), CODE_MOVE_CHARACTER_TO_WALL_FAILED_TAKE_MOVE)) {
            next.cell.offset = characterId;
            codeset.addSuccessElseFailure(isSuccess = true, CODE_MOVE_CHARACTER_TO_WALL_FAILED_ASSIGNMENT);

            // animate
            const Keyframe keyframe = wasFloored
                // floor -> wall
                ? Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, room0, prevFloor, direction)
                // wall -> wall
                : Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, room0, prevWall, direction);
            if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes), keyframe)) {
                // TODO: animation overflow
            }

            // this is the end of the movement!!!
        }
    }), CODE_INACCESSIBLE_NEIGHBORING_WALL);

    return isSuccess;
}
