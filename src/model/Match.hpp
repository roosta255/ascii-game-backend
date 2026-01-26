#pragma once

#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Cell.hpp"
#include "CodeEnum.hpp"
#include "Dungeon.hpp"
#include "Pointer.hpp"
#include "Titan.hpp"
#include "Turner.hpp"
#include <string>

class Codeset;
class GenerationParameters;
class JsonParameters;
namespace Json {
    class Value;
}

struct Match {
    constexpr static int MATCH_BUILDER_COUNT = 3;
    constexpr static long MOVE_ANIMATION_DURATION = 900;

    std::string filename = "ERROR_UNSET_FILENAME";
    std::string username;
    std::string host;
    uint32_t version = 0;
    int generator = 0;

    Titan titan;
    Array<Builder, MATCH_BUILDER_COUNT> builders;
    Dungeon dungeon;
    Turner turner;

    void setFilename();

    bool join(const std::string& joiner);
    bool leave(const std::string& leaver, CodeEnum& error);
    bool start();
    bool generate(int seed, Codeset& codeset);
    bool setupSingleBuilder(CodeEnum& output);
    bool setupSingleTitan(CodeEnum& output);
    CodeEnum moveCharacterToWall(int roomId, int characterId, Cardinal, Timestamp time);
    CodeEnum moveCharacterToWall(Room& room, Character& character, Cardinal, Timestamp time);
    bool moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time, CodeEnum& error);
    bool moveCharacterToFloor(Room& room, Character& character, Cell& floor, Timestamp time, CodeEnum& error);

    bool activateCharacter(Player& player, Character& character, Room& room, Pointer<Character> target, Codeset& codeset, Timestamp time = Timestamp());
    bool activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId, Codeset& codeset, Timestamp time = Timestamp());
    bool activateInventoryItem(Player& player, Character& character, Room& room, Item& item, Codeset& codeset, Timestamp time = Timestamp());
    bool activateInventoryItem(const std::string& playerId, int characterId, int roomId, int targetId, Codeset& codeset, Timestamp time = Timestamp());
    bool activateDoor(Player& player, Character& character, Room& room, Cardinal direction, Codeset& codeset, Timestamp time = Timestamp());
    bool activateDoor(const std::string& player, int characterId, int roomId, int direction, Codeset& codeset, Timestamp time = Timestamp());
    bool activateLock(Player& player, Character& character, Room& room, Cardinal direction, Codeset& codeset, Timestamp time = Timestamp());
    bool activateLock(const std::string& player, int characterId, int roomId, int direction, Codeset& codeset, Timestamp time = Timestamp());
    bool addCharacterToFloor(const Character& character, int roomId);

    bool endTurn(const std::string& playerId, CodeEnum& error);

    bool cleanupMovement(Character& character, Room& room, int& characterId, bool& wasFloored, int2& prevFloor, bool& wasWalled, Cardinal& prevWall, CodeEnum& error);
    bool isStarted(CodeEnum& result) const;
    bool isCompleted(CodeEnum& result) const;
    bool isFull(CodeEnum& result) const;
    bool isEmpty(CodeEnum& result) const;

    bool isCharacterInRoom(CodeEnum& error, Room& source, Character& offset);

    bool containsCharacter(const Character&, int& offset) const;
    bool containsOffset(int offset) const;
    bool containsCellCharacter(const Cell& cell) const;
    Pointer<Character> getCharacter(int offset, CodeEnum&);
    Pointer<Player> getPlayer(const std::string& player, CodeEnum&);
    bool accessPlayer(const std::string& player, CodeEnum&, std::function<void(Titan&)>, std::function<void(Builder&)>);

    bool accessPlayers(CodeEnum&, std::function<void(Titan&, Player&)>, std::function<void(Builder&, Player&, const int)>);

};
