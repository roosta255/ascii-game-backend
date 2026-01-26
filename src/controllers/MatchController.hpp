#pragma once

#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "Pointer.hpp"
#include "Timestamp.hpp"
#include <string>

class Codeset;
class Match;
class Player;
class Room;

struct MatchController {
private:
    Match& match;
    Codeset& codeset;

public:
    // constants
    constexpr static long MOVE_ANIMATION_DURATION = 900;
    
    // constructors
    MatchController(Match& match, Codeset& codeset);

    // functions
    bool activateCharacter(Player& player, Character& character, Room& room, Pointer<Character> target, Timestamp time = Timestamp());
    bool activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time = Timestamp());
    bool activateDoor(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time = Timestamp());
    bool activateDoor(const std::string& player, int characterId, int roomId, int direction, Timestamp time = Timestamp());
    bool activateInventoryItem(Player& player, Character& character, Room& room, Item& item, Timestamp time = Timestamp());
    bool activateInventoryItem(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time = Timestamp());
    bool activateLock(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time = Timestamp());
    bool activateLock(const std::string& player, int characterId, int roomId, int direction, Timestamp time = Timestamp());

    bool cleanupMovement(Character& character, Room& room, int& characterId, bool& wasFloored, int2& prevFloor, bool& wasWalled, Cardinal& prevWall);

    bool generate(int seed);

    bool moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time = Timestamp());
    bool moveCharacterToFloor(Room& room, Character& character, Cell& floor, Timestamp time = Timestamp());
    bool moveCharacterToWall(int roomId, int characterId, Cardinal, Timestamp time = Timestamp());
    bool moveCharacterToWall(Room& room, Character& character, Cardinal, Timestamp time = Timestamp());

};
