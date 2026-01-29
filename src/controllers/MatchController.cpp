#include "ChannelEnum.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int3.hpp"
#include "ItemFlyweight.hpp"
#include "Keyframe.hpp"
// #include "make_filename.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RoleFlyweight.hpp"

MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset) {}

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

bool MatchController::activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time) {

    bool isSuccess = false;

    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                // NOTE: this error code isn't captured as a failure/success
                // this lets the iActivator needs to decide whether thats a problem
                CodeEnum error = CODE_UNSET;
                auto target = match.getCharacter(targetId, error);
                codeset.addFailure(!(isSuccess = activateCharacter(player, character, room, target, time)));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::activateCharacter(Player& player, Character& subject, Room& room, Pointer<Character> target, Timestamp time) {
    setupLocations(false);

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character can take an action
    if (codeset.addFailure(!subject.isActionable(codeset.error))) return false;

    int subjectOffset;
    if(codeset.addFailure(!match.containsCharacter(subject, subjectOffset), CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID)) {
        return false;
    }
    if (codeset.addFailure(!isCharacterWithinRoom(subject, room, codeset), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isTargetValid = false;
    target.access([&](Character& target) {
        int targetOffset;
        if(codeset.addFailure(!match.containsCharacter(target, targetOffset), CODE_INACCESSIBLE_TARGET_CHARACTER_ID)) {
            return;
        }
        if (codeset.addFailure(!isCharacterWithinRoom(target, room, codeset), CODE_TARGET_CHARACTER_NOT_IN_ROOM)) {
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
                Activation activation(player, subject, room, target, Cardinal::north(), match, codeset, *this, time);
                codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
            });
        }
    }), CODE_INACCESSIBLE_ROLE_FLYWEIGHT);

    return isSuccess;
}

bool MatchController::activateDoor(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time) {
    setupLocations(false);

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character is an actor
    if (codeset.addFailure(!character.isActor(codeset.error))) {
        return false;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);
    return !codeset.addFailure(!wall.activateDoor(player, character, room, direction, match, codeset, *this, time));
}

bool MatchController::activateDoor(const std::string& playerId, int characterId, int roomId, int direction, Timestamp time) {
    bool isSuccess = false;
    // Get the player
    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        // Get the character
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            // Get the room
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                codeset.addFailure(!(isSuccess = activateDoor(player, character, room, Cardinal(direction), time)));
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
                    codeset.addFailure(!(isSuccess = activateInventoryItem(player, character, room, item, time)));
                }));
            }));
        }));
    }));

    return isSuccess;
}

bool MatchController::activateInventoryItem(Player& player, Character& subject, Room& room, Item& item, Timestamp time) {
    setupLocations(false);

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character can take an action
    if (codeset.addFailure(!subject.isActor(codeset.error))) return false;

    int subjectOffset;
    if (codeset.addFailure(!match.containsCharacter(subject, subjectOffset), CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID)) {
        return false;
    }
    if (codeset.addFailure(!isCharacterWithinRoom(subject, room, codeset), CODE_SUBJECT_CHARACTER_NOT_IN_ROOM)) {
        return false;
    }

    bool isSuccess = false;
    codeset.addFailure(!item.accessFlyweight([&](const ItemFlyweight& flyweight){
        flyweight.useActivator.accessConst([&](const iActivator& activatorIntf){
            Activation activation(player, subject, room, item, Cardinal::north(), match, codeset, *this, time);
            codeset.addFailure(!(isSuccess = activatorIntf.activate(activation)));
        });
    }), CODE_INACCESSIBLE_INVENTORY_FLYWEIGHT);

    return isSuccess;
}

bool MatchController::activateLock(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time) {
    setupLocations(false);

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    // Check if character is an actor
    if (codeset.addFailure(!character.isActor(codeset.error))) {
        return false;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);
    return !codeset.addFailure(!wall.activateLock(player, character, room, direction, match, codeset, *this, time));
}

bool MatchController::activateLock(const std::string& playerId, int characterId, int roomId, int direction, Timestamp time) {
    bool isSuccess = false;
    // Get the player
    codeset.addFailure(!match.getPlayer(playerId, codeset.error).access([&](Player& player) {
        // Get the character
        codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character) {
            // Get the room
            codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
                codeset.addFailure(!(isSuccess = activateLock(player, character, room, Cardinal(direction), time)));
            }));
        }));
    }));

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

const Map<int3, int>& MatchController::getDoors() {
    setupLocations(false);
    return doors;
}

const Map<int3, int>& MatchController::getFloors() {
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

bool MatchController::isCharacterWithinRoom(const Character& character, const Room& room, Codeset& codeset2) const {
    int roomId;
    if (codeset2.addFailure(!match.dungeon.containsRoom(room, roomId), CODE_ROOM_NOT_WITHIN_DUNGEON)) {
        return false;
    }

    int neighborId = -1;
    switch(character.location.type) {
        case LOCATION_DOOR:
            return character.location.roomId == roomId;
        case LOCATION_FLOOR:
            return character.location.roomId == roomId;
        case LOCATION_SHAFT_BOTTOM:
            if (character.location.roomId == roomId) {
                return true;
            }
            return character.location.roomId == room.above;
        case LOCATION_SHAFT_TOP:
            if (character.location.roomId == roomId) {
                return true;
            }
            return character.location.roomId == room.below;
        case LOCATION_DOOR_SHARED:
            if (character.location.roomId == roomId) {
                return true;
            }
            return character.location.roomId == room.getWall(Cardinal(character.location.data).getFlip()).adjacent;
    }
    return false;
}

bool MatchController::isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId, const Match& match, const Map<int3, int>& doors) {
    outCharacterId = doors.getOrDefault(int3{roomId, channel, dir.getIndex()}, -1);
    return match.containsOffset(outCharacterId);
}

bool MatchController::isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId) {
    return isDoorOccupied(roomId, channel, dir, outCharacterId, match, doors);
}

bool MatchController::isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId, const Match& match, const Map<int3, int>& floors) {
    outCharacterId = floors.getOrDefault(int3{roomId, channel, floorId}, -1);
    return match.containsOffset(outCharacterId);
}

bool MatchController::isFloorOccupied(int roomId, ChannelEnum channel, int floorId,int& outCharacterId) {
    return isFloorOccupied(roomId, channel, floorId, outCharacterId, match, floors);
}

bool MatchController::moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time) {
    bool result = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character){
        codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
            codeset.addFailure(!(result = moveCharacterToFloor(room, character, floorId, time)), CODE_MOVE_CHARACTER_TO_FLOOR_FAILED);
        }));
    }));
    return result;
}

bool MatchController::moveCharacterToFloor(Room& room, Character& character, int floorId, Timestamp time) {

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error), CODE_MATCH_NOT_STARTED)) {
        return false;
    }

    int roomId;
    if (codeset.addFailure(!match.dungeon.containsRoom(room, roomId), CODE_ROOM_NOT_WITHIN_DUNGEON)) {
        return false;
    }

    int characterId;
    if (codeset.addFailure(!match.containsCharacter(character, characterId))) {
        return false;
    }

    // Check for occupied target cell
    int conflictingCharacterId;
    if (codeset.addFailure(isFloorOccupied(roomId, character.location.channel, floorId, conflictingCharacterId), CODE_OCCUPIED_TARGET_FLOOR_CELL)) {
        return false;
    }

    // take move, first mutation so no going back
    if (codeset.addFailure(!character.takeMove(codeset.error))) {
        return false;
    }

    const auto newLocation = Location::makeFloor(roomId, character.location.channel, floorId);

    Location oldLocation;
    codeset.addFailure(!updateCharacterLocation(character, newLocation, oldLocation), CODE_UPDATE_CHARACTER_LOCATION_FAILED);

    const auto keyframe = Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, oldLocation, newLocation, codeset);
    if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes), keyframe)) {
        codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
    }

    return true;
}

bool MatchController::moveCharacterToWall(int roomId, int characterId, Cardinal direction, Timestamp time) {
    bool isSuccess = false;
    codeset.addFailure(!match.getCharacter(characterId, codeset.error).access([&](Character& character){
        codeset.addFailure(!match.dungeon.getRoom(roomId, codeset.error).access([&](Room& room) {
            codeset.addFailure(!(isSuccess = moveCharacterToWall(room, character, direction, time)), CODE_MOVE_CHARACTER_TO_WALL_FAILED);
        }), CODE_INACCESSIBLE_ROOM_ID);
    }), CODE_INACCESSIBLE_CHARACTER_ID);

    return isSuccess;
}

bool MatchController::moveCharacterToWall(Room& room, Character& character, Cardinal direction, Timestamp time) {
    
    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error), CODE_MATCH_NOT_STARTED)) {
        return false;
    }

    int roomId;
    if (codeset.addFailure(!match.dungeon.containsRoom(room, roomId), CODE_ROOM_NOT_WITHIN_DUNGEON)) {
        return false;
    }

    int characterId;
    if (codeset.addFailure(!match.containsCharacter(character, characterId))) {
        return false;
    }

    // Check for occupied target cell
    int conflictingCharacterId;
    if (codeset.addFailure(isDoorOccupied(roomId, character.location.channel, direction, conflictingCharacterId), CODE_OCCUPIED_TARGET_WALL_CELL)) {
        return false;
    }

    Wall& next = room.getWall(direction);
    if (codeset.addFailure(!next.isWalkable(codeset.error), CODE_DOOR_FAILED_WALKABILITY)) {
        return false;
    }

    // take move, first mutation so no going back
    if (codeset.addFailure(!character.takeMove(codeset.error))) {
        return false;
    }


    // most doors do share a wall, and it can be assumed that if the next room 1) exists 2) is walkable, then that's a 2-way door.
    bool isShared = false;
    match.dungeon.rooms.access(next.adjacent, [&](Room& room2){
        // hide this error because it's not a problem
        codeset.addFailure(!room2.getWall(direction.getFlip()).readIsSharedDoorway(codeset.error, isShared));
    });

    const auto newLocation = isShared ? Location::makeSharedDoor(next.adjacent, character.location.channel, direction.getFlip()) : Location::makeDoor(roomId, character.location.channel, direction);

    Location oldLocation;
    updateCharacterLocation(character, newLocation, oldLocation);

    const auto keyframe = Keyframe::buildWalking(time, MOVE_ANIMATION_DURATION, oldLocation, newLocation, codeset);
    if(!Keyframe::insertKeyframe(Rack<Keyframe>::buildFromArray<Character::MAX_KEYFRAMES>(character.keyframes), keyframe)) {
        codeset.addLog(CODE_ANIMATION_OVERFLOW_IN_MOVE_CHARACTER_TO_FLOOR);
    }

    return true;
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
