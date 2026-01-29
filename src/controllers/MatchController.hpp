#pragma once

#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "int3.hpp"
#include "int4.hpp"
#include "Map.hpp"
#include "Pointer.hpp"
#include <string>
#include "Timestamp.hpp"

class Codeset;
class Inventory;
class Match;
class Player;
class Room;

struct MatchController {
private:
    // members
    Match& match;
public:
    Codeset& codeset;
private:
    Map<int3, int> floors;
    Map<int3, int> doors;

    bool isLocationsSetup = false;
public:
    // constants
    constexpr static long MOVE_ANIMATION_DURATION = 900;
    
    // constructors
    MatchController(Match& match, Codeset& codeset);

    // functions
    bool addCharacterToFloor(const Character& character, int roomId, ChannelEnum channel, int& characterId);
    bool activateCharacter(Player& player, Character& character, Room& room, Pointer<Character> target, Timestamp time = Timestamp());
    bool activateCharacter(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time = Timestamp());
    bool activateDoor(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time = Timestamp());
    bool activateDoor(const std::string& player, int characterId, int roomId, int direction, Timestamp time = Timestamp());
    bool activateInventoryItem(Player& player, Character& character, Room& room, Item& item, Timestamp time = Timestamp());
    bool activateInventoryItem(const std::string& playerId, int characterId, int roomId, int targetId, Timestamp time = Timestamp());
    bool activateLock(Player& player, Character& character, Room& room, Cardinal direction, Timestamp time = Timestamp());
    bool activateLock(const std::string& player, int characterId, int roomId, int direction, Timestamp time = Timestamp());
    bool assignCharacterToFloor(int characterId, int roomId, ChannelEnum channel, int floorId);

    bool findFreeFloor(int roomId, ChannelEnum channel, int& output);
    bool generate(int seed);
    const Map<int3, int>& getDoors();
    const Map<int3, int>& getFloors();

    bool giveInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun = false);

    bool isCharacterActorValidation(const Character& character, const bool isCheckingCount = true);
    bool isCharacterMoverValidation(const Character& character, const bool isCheckingCount = true);
    bool isCharacterKeyerValidation(const Character& character);

    bool isCharacterWithinRoom(const Character& character, const Room& room, Codeset& codeset2) const;
    static bool isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId, const Match& match, const Map<int3, int>& doors);
    static bool isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId, const Match& match, const Map<int3, int>& floors);
    bool isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId);
    bool isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId);

    bool moveCharacterToFloor(int roomId, int characterId, int floorId, Timestamp time = Timestamp());
    bool moveCharacterToFloor(Room& room, Character& character, int floorId, Timestamp time = Timestamp());
    bool moveCharacterToWall(int roomId, int characterId, Cardinal, Timestamp time = Timestamp());
    bool moveCharacterToWall(Room& room, Character& character, Cardinal, Timestamp time = Timestamp());

    void setupLocations(bool isForced = false);
    bool takeCharacterAction(Character& character);
    bool takeCharacterMove(Character& character);
    bool takeInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun = false);
    bool updateCharacterLocation(Character& character, const Location& newLocation, Location& oldLocation);

    bool validateDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);
    bool validateSharedDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);
};
