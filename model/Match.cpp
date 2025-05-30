#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "Match.hpp"
#include <drogon/drogon.h>
#include <json/json.h>

constexpr size_t KILOBYTE = 1024;
static_assert(sizeof(Match) > KILOBYTE, "size milestone passed");
static_assert(sizeof(Match) < KILOBYTE * KILOBYTE, "size milestone passed");

bool Match::create(const std::string& creator) {
    static int idCounter = 0;

    std::ostringstream oss;
    oss << "match_" << ++idCounter;
    match = oss.str();
    version = 0;

    host = creator;
    titan.player.account = creator;

    return true;
}

bool Match::generate (int seed, Match& output) {
    const Match* me = this;
    bool success = false;
    GeneratorFlyweight::getFlyweights().accessConst(generator, [&](const GeneratorFlyweight& flyweight){
        flyweight.generator.accessConst([&](const iGenerator& generator){
            success = generator.generate(*me, seed, output);
        });
    });
    return success;
}

bool Match::join
(
    const std::string& joiner
) {
    bool joined = false;
    for (auto& builder: builders) {
        if (builder.player.account.empty()) {
            builder.player.account = joiner;
            joined = true;
            break;
        }
    }

    return joined;
}

bool Match::start() {
    if (started) {
        LOG_ERROR << "Match already started.";
        return false;
    }

    bool success = false;

    bool isDungeonAccessed = dungeon.accessLayout([&](const iLayout& layout){
        Room& entrance = layout.getEntrance(dungeon.rooms);
        entrance.floorCells.access(0, [&](Cell& cell){
            builders.access(0, [&](Builder& builder){
                if (!containsCharacter(builder.character, cell.offset)) {
                    LOG_ERROR << "Match started, but first builder not in match. Please investigate.";
                    return;
                }
                LOG_DEBUG << "Started character " << cell.offset;
                success = true;
            });
        });
    });

    if (!success) {
        LOG_ERROR << "Match::start(), but failed to move builder.";
    }

    return started = success;
}

MovementEnum Match::moveCharacterToWall
(
    int roomId,
    int characterId,
    Cardinal direction
) {
    // TODO: error swallowing is hard to follow
    LOG_DEBUG << "Match::moveCharacterToWall() starting...";
    if (!started) {
        LOG_DEBUG << "Match::moveCharacterToWall() failed due to unstarted match " << match;
        return MOVEMENT_UNSTARTED_MATCH;
    }

    MovementEnum result = MOVEMENT_SUCCESS;

    const bool isCharacterIdValid = getCharacter(characterId).access([&](Character& character){
        if (!character.isMovable()) {
            LOG_DEBUG << "Match::moveCharacterToWall() failed due to immovable character " << characterId;
            result = MOVEMENT_IMMOVABLE_CHARACTER;
            return;
        }

        const bool isRoomIdValid = dungeon.rooms.access(roomId, [&](Room& room) {
            // check next room
            Wall& next = room.getWall(direction);

            // occupied target check
            if (containsOffset(next.cell.offset)) {
                LOG_DEBUG << "Match::moveCharacterToWall() failed due to occupied target cell";
                result = MOVEMENT_OCCUPIED_TARGET_CELL;
                return;
            }

            bool isBlocked = true;
            room.getWall(direction).accessDoor([&](const DoorFlyweight& flyweight){
                isBlocked = flyweight.blocking;
            });
            if (isBlocked) {
                LOG_DEBUG << "Match::moveCharacterToWall() failed due to blocked door type";
                result = MOVEMENT_BLOCKING_DOOR_TYPE;
                return;
            }

            // cleanup previous
            bool isCleanedUp = false;
            dungeon.findCharacter(
                room,
                characterId,
                [&](Cell& local, Cardinal, Room&, Cell& other) {
                    local.offset = 0;
                    other.offset = 0;
                    LOG_DEBUG << "Match::moveCharacterToWall() cleaning up character from wall";
                    isCleanedUp = true;
                },
                [&](int, int, Cell& cell) {
                    cell.offset = 0;
                    LOG_DEBUG << "Match::moveCharacterToWall() cleaning up character from floor";
                    isCleanedUp = true;
                }
            );

            if (!isCleanedUp) {
                result = MOVEMENT_MISSING_CHARACTER_CLEANUP;
                return;
            }

            // set next
            next.cell.offset = characterId;
            LOG_DEBUG << "Match::moveCharacterToWall() setting cheracter in this room";
            const bool isLayoutValid = dungeon.accessLayout([&](const iLayout& layout){
                const bool isWallNeighborValid = layout.getWallNeighbor(dungeon.rooms, room, direction).access([&](Room& room2){
                    room2.getWall(direction.getFlip()).cell.offset = characterId;
                    LOG_DEBUG << "Match::moveCharacterToWall() setting cheracter in other room";
                });
                if (!isWallNeighborValid) {
                    LOG_DEBUG << "Match::moveCharacterToWall() failed due to innacessible neighboring wall";
                    result = MOVEMENT_INACCESSIBLE_NEIGHBORING_WALL;
                    return;
                }
            });
            if (!isLayoutValid) {
                LOG_DEBUG << "Match::moveCharacterToWall() failed due to innacessible layout";
                result = MOVEMENT_INACCESSIBLE_LAYOUT;
                return;
            }
        });
        if (!isRoomIdValid) {
            LOG_DEBUG << "Match::moveCharacterToWall() failed due to innacessible roomId " << roomId;
            result = MOVEMENT_INACCESSIBLE_ROOM_ID;
        }
    });
    if (result != MOVEMENT_SUCCESS) {
        // swallowed error check
        return result;
    }

    if (!isCharacterIdValid) {
        LOG_DEBUG << "Match::moveCharacterToWall() failed due to innacessible characterId" << characterId;
        return MOVEMENT_INACCESSIBLE_CHARACTER_ID;
    }

    return result;
}

bool Match::containsCharacter
(
    const Character& source,
    int& offset
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
(
    int offset
) const {
    return offset > 0 && offset < sizeof(Match);
}

Pointer<Character> Match::getCharacter (int offset) {
    // Compute absolute address from offset
    auto base = reinterpret_cast<char*>(this);
    auto ptr = base + offset;

    if (containsOffset(offset))
        // Cast back to Character pointer
        return Pointer<Character>(*reinterpret_cast<Character*>(ptr));
    else
        return Pointer<Character>::empty();
}

void Match::toJson(Json::Value& out) const {
    out["match"] = match;
    out["host"] = host;
    out["version"] = version;
    out["turn"] = turn;
    out["started"] = started;

    Json::Value dungonJson;
    dungeon.toJson(dungonJson);
    out["dungeon"] = dungonJson;

    Json::Value titanJson;
    titan.toJson(titanJson);
    out["titan"] = titanJson;

    Json::Value buildersJson(Json::arrayValue);
    for (const auto& builder: builders) {
        Json::Value b;
        builder.toJson(b);
        buildersJson.append(b);
    }
    out["builders"] = buildersJson;

    // generator
    GeneratorFlyweight::getFlyweights().accessConst(generator, [&](const GeneratorFlyweight& flyweight){
        out["generator"] = flyweight.name;
    });
}

bool Match::fromJson(const Json::Value& in) {
    // match
    if (!in.isMember("match")) {
        LOG_ERROR << "Match json is missing match field";
        return false;
    }
    match = in["match"].asString();

    if (!in.isMember("version")) {
        LOG_ERROR << "Match " << match << " json is missing version field";
        return false;
    }

    host = in["host"].asString();
    version = in["version"].asUInt();
    turn = in["turn"].asUInt();
    started = in["started"].asBool();

    if (!titan.fromJson(in["titan"])) {
        LOG_ERROR << "Match " << match << " json couldnt parse titan field";
        return false;
    }

    if (!dungeon.fromJson(in["dungeon"])) {
        LOG_ERROR << "Match " << match << " json couldnt parse dungeon field";
        return false;
    }

    const auto& builderJson = in["builders"];
    if (!builderJson.isArray()) {
        LOG_ERROR << "Match " << match << " json couldnt parse builders field due to it not being an array";
        return false;
    }

    if (builderJson.size() != MATCH_BUILDER_COUNT) {
        LOG_ERROR << "Match " << match << " json couldnt parse builders field due to it not having " << MATCH_BUILDER_COUNT << " elements";
        return false;
    }

    int i = 0;
    for (auto& builder: builders) {
        if (!builder.fromJson(builderJson[i])) {
            LOG_ERROR << "Match " << match << " json couldnt builders["<< i <<"] field";
            return false;
        }
        i++;
    }

    // generator
    if (!in.isMember("generator")) {
        LOG_ERROR << "Dungeon json is missing generator field";
        return false;
    }
    auto generatorName = in["generator"].asString();
    if (!GeneratorFlyweight::indexByString(generatorName, generator)) {
        LOG_ERROR << "Dungeon json couldnt parse generator " << generatorName;
        return false;
    }

    return true;
}
