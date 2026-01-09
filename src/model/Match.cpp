#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int2.hpp"
#include "JsonParameters.hpp"
#include "Keyframe.hpp"
#include "make_filename.hpp"
#include "Match.hpp"
#include "RoleFlyweight.hpp"
#include <json/json.h>
#include <nlohmann/json.hpp>

constexpr size_t KILOBYTE = 1024;
static_assert(sizeof(Match) > KILOBYTE, "size milestone passed");
static_assert(sizeof(Match) < KILOBYTE * KILOBYTE, "size milestone passed");

void Match::setFilename() {
    filename = make_filename("match-" + host + "-" + username);
}

bool Match::generate
(int seed, CodeEnum& error) {
    bool success = false;
    GeneratorFlyweight::getFlyweights().accessConst(generator, [&](const GeneratorFlyweight& flyweight){
        flyweight.generator.accessConst([&](const iGenerator& generator){
            success = generator.generate(seed, *this);
        });
    });
    return success;
}

bool Match::accessPlayer
(const std::string& player, CodeEnum& error, std::function<void(Titan&)> titanConsumer, std::function<void(Builder&)> builderConsumer) {
    if (titan.player.account == player) {
        titanConsumer(titan);
        return true;
    }
    for (auto& builder: builders) {
        if (builder.player.account == player) {
            builderConsumer(builder);
            return true;
        }
    }
    error = CODE_INACCESSIBLE_PLAYER;
    return false;
}

bool Match::accessPlayers
( CodeEnum& error, std::function<void(Titan&, Player&)> consumeTitan, std::function<void(Builder&, Player&, const int)> consumeBuilder)
{
    if (!titan.player.isEmpty())
        consumeTitan(titan, titan.player);
    int i = 0;
    for (auto& builder: builders) {
        if (!builder.player.isEmpty())
            consumeBuilder(builder, builder.player, i++);
    }
    return true;
}

bool Match::setupSingleBuilder(CodeEnum& error) {
    // assuming TITAN_ONLY, addBuilder, removeTitan
    // migrate host to builder[0]
    turner.addBuilder();
    turner.removeTitan();
    accessPlayers(error,
        [](Titan&, Player& player){ player.account = ""; },
        [](Builder&, Player& player, const int){
            player.account = "";
        });
    builders.access(0, [&](Builder& builder){ builder.player.account = this->host; });
    return true;
}

bool Match::setupSingleTitan(CodeEnum& error) {
    // assuming TITAN_ONLY, noop
    // migrate host to titan
    return accessPlayers(error,
        [&](Titan&, Player& player){ player.account = this->host; },
        [](Builder&, Player& player, const int){ player.account = ""; });
}

bool Match::join(const std::string& joiner) {

    if (this->titan.player.account.empty()) {
        this->titan.player.account = joiner;
        turner.addTitan();
        return true;
    }

    for (auto& builder: builders) {
        if (builder.player.account.empty()) {
            builder.player.account = joiner;
            turner.addBuilder();
            return true;
        }
    }

    return false;
}

bool Match::start() {
    CodeEnum error;
    if (isStarted(error)) {
        return false;
    }

    return turner.startGame() == CODE_SUCCESS;
}

bool Match::cleanupMovement(Character& character, Room& room, int& characterId, bool& wasFloored, int2& prevFloor, bool& wasWalled, Cardinal& prevWall, CodeEnum& error) {
    if (!isStarted(error)) return false;

    // Check for movability
    if (!character.isMovable(error, true)) return false;

    // Check if character is in room
    if (!isCharacterInRoom(error, room, character)) return false;

    if (!containsCharacter(character, characterId)) {
        error = CODE_INACCESSIBLE_CHARACTER_ID;
        return false;
    }

    // Cleanup previous
    bool isCleanedUp = false;
    wasWalled = false;
    wasFloored = false;
    dungeon.findCharacter(
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

    if (!isCleanedUp) error = CODE_MISSING_CHARACTER_CLEANUP;
    return isCleanedUp;
}

CodeEnum Match::moveCharacterToWall(int roomId, int characterId, Cardinal direction, Timestamp time) {
    CodeEnum result = CODE_PREACTIVATE_IN_LOOKUPS;

    getCharacter(characterId, result).access([&](Character& character){
        dungeon.getRoom(roomId, result).access([&](Room& room) {
            result = moveCharacterToWall(room, character, direction, time);
        });
    });

    return result;
}

CodeEnum Match::moveCharacterToWall(Room& room, Character& character, Cardinal direction, Timestamp time) {
    // TODO: Check if match has started
    CodeEnum result = CODE_PREACTIVATE_IN_CHECKS;

    int room0;
    if (!dungeon.containsRoom(room, room0)) {
        return CODE_ROOM_NOT_WITHIN_DUNGEON;
    }

    // Check for occupied target cell
    Wall& next = room.getWall(direction);
    if (containsOffset(next.cell.offset)) return CODE_OCCUPIED_TARGET_WALL_CELL;

    if (!next.isWalkable(result)) return result;

    int characterId;
    bool wasWalled, wasFloored;
    int2 prevFloor;
    Cardinal prevWall = Cardinal::north();
    if (!cleanupMovement(character, room, characterId, wasFloored, prevFloor, wasWalled, prevWall, result)) return result;

    // set next
    result = CODE_PREACTIVATE_IN_PROCESS;
    next.cell.offset = characterId;
    dungeon.accessLayout(result, [&](const iLayout& layout){
        const bool isWallNeighborValid = layout.getWallNeighbor(dungeon.rooms, room, direction).access([&](Room& room2){
            // this is where the actual move happens
            room2.getWall(direction.getFlip()).cell.offset = characterId;
            if (character.takeMove(result)) {
                result = CODE_SUCCESS;

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
                return;
            }
        });
        if (!isWallNeighborValid) {
            result = CODE_INACCESSIBLE_NEIGHBORING_WALL;
            return;
        }
    });

    return result;
}

bool Match::isCharacterInRoom(CodeEnum& error, Room& source, Character& offset) {
    int characterId;
    if (!containsCharacter(offset, characterId)) {
        error = CODE_INACCESSIBLE_CHARACTER_ID;
        return false;
    }

    if (dungeon.findCharacter(
        source,
        characterId,
        [&](Cell& local, Cardinal, Room&, Cell& other) {},
        [&](int, int, Cell& cell) {}
    )) {
        return true;
    }

    error = CODE_CHARACTER_NOT_IN_ROOM;
    return false;
}

bool Match::containsCharacter
( const Character& source, int& offset
) const {
    // Get base address of this match object as a byte pointer
    auto base = reinterpret_cast<const char*>(this);

    // Get address of the source character
    auto ptr = reinterpret_cast<const char*>(&source);

    // Verify that the character lies within this Match object
    auto end = base + sizeof(Match);
    if (ptr >= base && ptr < end) {
        // Calculate offset
        offset = static_cast<int>(ptr - base);
        return true;
    }
    return false;
}

bool Match::containsOffset
( int offset
) const {
    return offset > 0 && offset < sizeof(Match);
}

bool Match::containsCellCharacter
( const Cell& cell
) const {
    return containsOffset(cell.offset);
}

Pointer<Character> Match::getCharacter (int offset, CodeEnum& error) {
    // Compute absolute address from offset
    auto base = reinterpret_cast<char*>(this);
    auto ptr = base + offset;

    if (containsOffset(offset))
        // Cast back to Character pointer
        return Pointer<Character>(*reinterpret_cast<Character*>(ptr));
    error = CODE_INACCESSIBLE_CHARACTER_ID;
    return Pointer<Character>::empty();
}

Pointer<Player> Match::getPlayer(const std::string& player, CodeEnum& error) {
    for (auto& builder: builders) {
        if (builder.player.account == player) {
            return builder.player;
        }
    }
    if (titan.player.account == player) {
        return titan.player;
    }
    error = CODE_INACCESSIBLE_PLAYER;
    return Pointer<Player>::empty();
}

CodeEnum Match::activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId) {
    CodeEnum result = CODE_PREACTIVATE_IN_LOOKUPS;

    getPlayer(playerId, result).access([&](Player& player) {
        getCharacter(characterId, result).access([&](Character& character) {
            dungeon.getRoom(roomId, result).access([&](Room& room) {
                auto target = getCharacter(targetId, result);
                result = activateCharacter(player, character, room, target);
            });
        });
    });

    return result;
}

CodeEnum Match::activateCharacter(Player& player, Character& subject, Room& room, Pointer<Character> target) {
    CodeEnum result = CODE_PREACTIVATE_IN_MATCH;

    // Check if match has started
    if (!isStarted(result)) return result;

    // Check if character can take an action
    if (!subject.isActionable(result)) return result;

    int subjectOffset;
    if(!containsCharacter(subject, subjectOffset)) {
        return CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID;
    }
    if (!room.containsCharacter(subjectOffset)) {
        return CODE_SUBJECT_CHARACTER_NOT_IN_ROOM;
    }

    bool isTargetValid = false;
    target.access([&](Character& target) {
        int targetOffset;
        if(!containsCharacter(target, targetOffset)) {
            result = CODE_INACCESSIBLE_TARGET_CHARACTER_ID;
            return;
        }
        if (!room.containsCharacter(targetOffset)) {
            result = CODE_TARGET_CHARACTER_NOT_IN_ROOM;
            return;
        }
        isTargetValid = true;
    });

    if (!isTargetValid) {
        return result;
    }

    subject.accessRole(result, [&](const RoleFlyweight& flyweight) {
        if (flyweight.activator.isEmpty()) {
            result = CODE_MISSING_ACTIVATOR;
            return;
        } else {
            flyweight.activator.accessConst([&](const iActivator& activatorIntf){
                Activation activation(player, subject, room, target, Cardinal::north(), *this);
                result = activatorIntf.activate(activation);
            });
        }
    });

    return result;
}

bool Match::isStarted(CodeEnum& error) const {
    if (!turner.isStarted()) {
        error = CODE_UNSTARTED_MATCH;
        return false;
    }
    return true;
}

bool Match::isCompleted(CodeEnum& error) const {
    if (!turner.isCompleted()) {
        error = CODE_UNCOMPLETED_MATCH;
        return false;
    }
    return true;
}

bool Match::isFull(CodeEnum& error) const {
    if (this->titan.player.account.empty()) {
        return false;
    }
    for (const auto& builder: this->builders) {
        if (builder.player.account.empty()) {
            return false;
        }
    }
    return true;
}

bool Match::isEmpty(CodeEnum& error) const {
    if (!this->titan.player.account.empty()) {
        return false;
    }
    for (const auto& builder: this->builders) {
        if (!builder.player.account.empty()) {
            return false;
        }
    }
    return true;
}

bool Match::addCharacterToFloor(const Character& source, int roomId) {
    CodeEnum result;
    // Validate room ID and get room
    auto room = dungeon.getRoom(roomId, result);
    if (room.isEmpty()) return false;

    // Find an empty character slot
    int characterOffset;
    for (Character& character: dungeon.characters) {
        if (character.role == ROLE_EMPTY) {
            // Calculate the offset for the new character, guaranteed contained
            containsCharacter(character, characterOffset);
            character = source;
            break;
        }
    }

    // Find an empty floor cell
    bool success = false;
    room.access([&](Room& room) {
        for (Cell& cell: room.floorCells) {
            if (!containsOffset(cell.offset)) {
                // Set the character offset in the cell
                cell.offset = characterOffset;
                success = true;
                if (success) break;
            }
        }
    });

    // If we couldn't find an empty cell, reset the character slot
    if (!success) {
        getCharacter(characterOffset, result).access([&](Character& character) {
            character.role = ROLE_EMPTY;
        });
    }

    return success;
}

CodeEnum Match::activateLock(Player& player, Character& character, Room& room, Cardinal direction) {
    CodeEnum result = CODE_PREACTIVATE_IN_MATCH;

    // Check if match has started
    if (!isStarted(result)) return result;

    // Check if character is an actor
    if (!character.isActor(result)) {
        return result;
    }

    // Get and validate the door
    Wall& wall = room.getWall(direction);
    result = wall.activate(player, character, room, direction, *this);

    return result;
}

CodeEnum Match::activateLock(const std::string& playerId, int characterId, int roomId, int direction) {
    // Get the player
    CodeEnum result = CODE_PREACTIVATE_IN_LOOKUPS;
    getPlayer(playerId, result).access([&](Player& player) {
        // Get the character
        getCharacter(characterId, result).access([&](Character& character) {
            // Get the room
            dungeon.getRoom(roomId, result).access([&](Room& room) {
                result = activateLock(player, character, room, Cardinal(direction));
            });
        });
    });

    return result;
}

bool Match::leave(const std::string& playerId, CodeEnum& error) {
    return accessPlayer(playerId, error, [&](Titan& titan) {
        titan.player.account = "";
        turner.removeTitan();
    }, [&](Builder& builder) {
        builder.player.account = "";
        turner.removeBuilder(playerId, *this);
    });
}

bool Match::endTurn(const std::string& playerId, CodeEnum& error)
{
    return accessPlayer(playerId, error, [&](Titan& titan) {
        turner.endTitanTurn(*this);
    }, [&](Builder& builder) {
        turner.endBuilderTurn(*this);
    });
}

bool Match::moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time, CodeEnum& error) {
    bool result = false;
    getCharacter(characterId, error).access([&](Character& character){
        dungeon.getRoom(roomId, error).access([&](Room& room) {
            room.getCell(floorId, error).access([&](Cell& floor) {
                result = moveCharacterToFloor(room, character, floor, time, error);
            });
        });
    });
    return result;
}

bool Match::moveCharacterToFloor(Room& room, Character& character, Cell& floor, Timestamp time, CodeEnum& error) {

    int room0;
    if (!dungeon.containsRoom(room, room0)) {
        return CODE_ROOM_NOT_WITHIN_DUNGEON;
    }

    // Check for occupied target cell
    if (containsOffset(floor.offset)) {
        error = CODE_OCCUPIED_TARGET_FLOOR_CELL;
        return false;
    }

    // Cleanup previous
    int characterId;
    bool wasWalled, wasFloored;
    int2 prevFloor;
    Cardinal prevWall = Cardinal::north();
    if (!cleanupMovement(character, room, characterId, wasFloored, prevFloor, wasWalled, prevWall, error)) return false;

    if (!character.takeMove(error)) return false;

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

    return true;
}


CodeEnum Match::activateInventoryItem(const std::string& playerId, int characterId, int roomId, int itemId) {
    CodeEnum result = CODE_PREACTIVATE_IN_LOOKUPS;

    getPlayer(playerId, result).access([&](Player& player) {
        getCharacter(characterId, result).access([&](Character& character) {
            dungeon.getRoom(roomId, result).access([&](Room& room) {
                player.inventory.accessItem(itemId, result, [&](Item& item){
                    result = activateInventoryItem(player, character, room, item);
                });
            });
        });
    });

    return result;
}

CodeEnum Match::activateInventoryItem(Player& player, Character& subject, Room& room, Item& item) {
    CodeEnum result = CODE_PREACTIVATE_IN_MATCH;

    // Check if match has started
    if (!isStarted(result)) return result;

    // Check if character can take an action
    if (!subject.isActor(result)) return result;

    int subjectOffset;
    if(!containsCharacter(subject, subjectOffset)) {
        return CODE_INACCESSIBLE_SUBJECT_CHARACTER_ID;
    }
    if (!room.containsCharacter(subjectOffset)) {
        return CODE_SUBJECT_CHARACTER_NOT_IN_ROOM;
    }

    subject.accessRole(result, [&](const RoleFlyweight& flyweight) {
        if (flyweight.activator.isEmpty()) {
            result = CODE_MISSING_ACTIVATOR;
            return;
        } else {
            flyweight.activator.accessConst([&](const iActivator& activatorIntf){
                Activation activation(player, subject, room, item, Cardinal::north(), *this);
                result = activatorIntf.activate(activation);
            });
        }
    });

    return result;
}
