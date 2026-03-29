#pragma once

#include "ActionEnum.hpp"
#include "Activation.hpp"
#include "BehaviorEventEnum.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CodeEnum.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "EventFlyweight.hpp"
#include "int2.hpp"
#include "int3.hpp"
#include "int4.hpp"
#include "LoggedEvent.hpp"
#include "Map.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include <functional>
#include <string>
#include <vector>
#include "Timestamp.hpp"
#include "TraitBits.hpp"
#include "TraitModifier.hpp"

class CharacterAction;
class Codeset;
struct Chest;
class iActivator;
class Inventory;
class Match;
class Player;
struct Preactivation;
class Room;

// Event trigger entry for deferred processing after activations
struct PendingTrigger {
    const iActivator* activator = nullptr;
    int characterId = -1;
    int targetId = -1;
    // Set when this trigger is a behavioral AI response; identifies the acting agent.
    Maybe<int> agentId;
    // Event type used to dispatch AI behavior responses. BEHAVIOR_EVENT_NIL suppresses dispatch.
    BehaviorEventEnum eventType = BEHAVIOR_EVENT_NIL;
};

struct MatchController {
public:
    // members
    Match& match;
    Codeset& codeset;
private:
    // TODO: put both room maps in a struct
    Map<int, Map<int2, int> > floors; // roomId -> <channel, floorId> -> characterId
    Map<int, Map<int2, int> > doors; // roomId -> <channel, direction> -> characterId
    Map<int, TraitModifier::TraitComputation> traitsComputed; // characterId -> computed traits (always fresh, never persisted)
    Map<int, Pointer<Chest>> chestContainerMap; // containerCharacterId -> Chest
    std::vector<PendingTrigger>* eventQueuePtr = nullptr; // set during activate(); used by pushTrigger()
    Timestamp animationTime; // latest animation end time across all active activations
    bool isLocationsSetup = false;
public:
    // constants
    constexpr static long MOVE_ANIMATION_DURATION = 900;
    constexpr static long BOUNCE_LOCK_ANIMATION_DURATION = 2000;

    // constructors
    MatchController(Match& match, Codeset& codeset);

    // functions
    bool activate(const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog = Pointer<std::vector<LoggedEvent>>::empty());
    bool activate(const iActivator& activator, const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog = Pointer<std::vector<LoggedEvent>>::empty());
    bool allocateChest(int roomId, std::function<void(Chest&, Character&)> consumer);
    bool allocateChest(int roomId, std::function<void(Chest&, Character&, Character&)> consumer);
    bool allocateCharacterToFloor(int roomId, ChannelEnum channel, std::function<void(Character&)> consumer, int& outCharacterId, int& outFloorId);
    bool assignCharacterToFloor(int characterId, int roomId, ChannelEnum channel, int floorId);

    bool findFreeFloor(int roomId, ChannelEnum channel, int& output);
    bool findCharacterPath
        ( const std::string& playerId
        , int characterId
        , int loops
        , std::function<bool(const Match&)> destination
        , std::function<int(const CharacterAction&, const Match&)> heuristic
        , std::function<void(const CharacterAction&, const Match&)> consumer
        , const bool isFailure = true);

    bool generate(int seed);
    const Map<int, Map<int2, int> >& getDoors();
    const Map<int, Map<int2, int> >& getFloors();
    Pointer<Chest> getChestByContainerId(int characterId);

    bool giveInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun = false);
    bool giveInventoryItem(Inventory& inventory, const ItemEnum type, const Timestamp& time, const int roomId, const bool isSkippingAnimations);

    bool isCharacterActorValidation(const Character& character, const bool isCheckingCount = true);
    bool isCharacterMoverValidation(const Character& character, const bool isCheckingCount = true);
    bool isCharacterKeyerValidation(const Character& character);

    bool isCharacterWithinRoom(int characterId, int roomId, bool& result);
    static bool isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId, const Match& match, const Map<int, Map<int2, int> >& doors);
    static bool isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId, const Match& match, const Map<int, Map<int2, int> >& floors);
    bool isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId);
    bool isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId);

    bool permuteCharacterActions(const std::string& playerId, int characterId, std::function<void(const CharacterAction&, const Match&)> consumer);

    void setupLocations(bool isForced = false);
    void sortFloorCharacters(int roomId);
    bool takeCharacterAction(Character& character);
    bool takeCharacterMove(Character& character);
    bool giveCharacterAction(Character& character);
    bool giveCharacterMove(Character& character);
    bool breakArmorItem(Character& character);
    bool takeInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun = false);
    bool takeInventoryItem(Inventory& inventory, const ItemEnum type, const Timestamp& time, const int roomId, const bool isSkippingAnimations);
    bool updateCharacterLocation(Character& character, const Location& newLocation, Location& oldLocation);

    void updateTraits(Character& character);
    TraitModifier::TraitComputation getTraitsComputed(int characterId) const;
    const Map<int, TraitModifier::TraitComputation>& getTraitsComputedMap() const;

    // Appends a logged event to the specified room's event log and to activation.request.eventLog.
    void addLoggedEvent(Activation& activation, int roomId, LoggedEvent event);
    // Appends a logged event only to activation.request.eventLog (not the room log). Use for failure events.
    void addRequestLoggedEvent(Activation& activation, LoggedEvent event);

    void pushTrigger(const iActivator* activator, int characterId, int targetId = -1, BehaviorEventEnum eventType = BEHAVIOR_EVENT_NIL);

    // Called by activators when they schedule an animation; advances the global animation clock.
    void setAnimationTime(const Timestamp& t);

    bool validateCharacterWithinRoom(int characterId, int roomId);
    bool validateDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);
    bool validateSharedDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);
};
