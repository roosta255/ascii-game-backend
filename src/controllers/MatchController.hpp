#pragma once

#include "ActionEnum.hpp"
#include "BehaviorEventEnum.hpp"
#include "Cardinal.hpp"
#include "Character.hpp"
#include "CharacterAllocSpec.hpp"
#include "CodeEnum.hpp"
#include "Conduct.hpp"
#include "DUNGEON_ROOM_COUNT.hpp"
#include "EventFlyweight.hpp"
#include "FieldController.hpp"
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

struct ActivationContext;
class CharacterAction;
class Codeset;
struct Chest;
class iActivator;
struct Inventory;
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
    FieldController fields; // room-indexed derived environmental state (always fresh, never persisted)
    Map<int, Pointer<Chest>> chestContainerMap; // containerCharacterId -> Chest
    Map<int, Pointer<Conduct>> conductMap; // characterId -> Conduct
    Pointer<std::vector<PendingTrigger>> eventQueuePtr; // set during activate()/tickNpcConducts(); used by pushTrigger()
    Timestamp animationTime; // latest animation end time across all active activations
    bool isLocationsSetup = false;
public:
    // Set to true for the duration of findCharacterPath(). Guards against re-entrant A* (TriggerEffectComputePath).
    bool isPathfindingActive = false;
    // When true, tickNpcConducts() is a no-op. Set independently or via findCharacterPath() params.
    bool suppressNpcConducts = false;
    // When true, drainEventQueue() skips the per-event conduct dispatch loop.
    bool suppressNpcEventResponse = false;
    // constants
    constexpr static long MOVE_ANIMATION_DURATION = 900;
    constexpr static long BOUNCE_LOCK_ANIMATION_DURATION = 2000;

    // constructors
    MatchController(Match& match, Codeset& codeset);

    // functions
    bool activate(const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog = Pointer<std::vector<LoggedEvent>>::empty());
    bool activate(const iActivator& activator, const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog = Pointer<std::vector<LoggedEvent>>::empty());
    bool allocate(const CharacterAllocSpec& spec, const AttachmentContext& attachment, int& outCharacterId);
    bool assignCharacterToFloor(int characterId, int roomId, ChannelEnum channel, int floorId);
    bool buildActivationContext(const Preactivation& preactivation, std::function<void(ActivationContext&)>);

    // Ends playerId's turn (titan or builder) on this controller's match, and
    // ticks NPC conducts once the titan's turn has actually completed for the
    // round. This is the single entry point that owns that decision.
    bool endTurn(const std::string& playerId, CodeEnum& error);
    // Directly ends the titan's turn on this controller's match, bypassing
    // playerId dispatch (for non-player-driven titans). Always ticks.
    CodeEnum endTitanTurn();
    // Directly ends a builder round on this controller's match, bypassing
    // playerId dispatch. Ticks only if this also ended the titan's (possibly
    // phantom, solo-mode) turn.
    CodeEnum endBuilderTurn();
    // Pure turn-state transition dispatched by playerId, with no tick side effect.
    // Static so callers advancing a speculative/simulated Match (pathfinding's
    // ACTION_END_TURN candidate, A* search) can do so without a MatchController
    // and without ticking NPC conducts on a hypothetical state.
    static bool advanceTurnState(Match& match, const std::string& playerId, CodeEnum& error, bool& outTitanTurnEnded);

    bool findFreeFloor(int roomId, ChannelEnum channel, int& output);

    // BFS flood fill over room connectivity using the dungeon's pathfinderCharacter.
    // Consumer is called once per newly-discovered room with the CharacterAction that
    // first crossed into it (action.direction identifies which doorway was used).
    // floodFillRoom starts from a single room; floodFillRoomBits starts from every
    // room whose bit is set in startRoomBits (bit i = room i).
    bool floodFillRoom(int startRoomId, std::function<void(const CharacterAction&, const Match&)> consumer);
    bool floodFillRoomBits(int startRoomBits, std::function<void(const CharacterAction&, const Match&)> consumer);

    bool findCharacterPath
        ( const std::string& playerId
        , int characterId
        , int loops
        , std::function<bool(const Match&)> destination
        , std::function<int(const CharacterAction&, const Match&)> heuristic
        , std::function<void(const CharacterAction&, const Match&)> consumer
        , const bool isFailure = true
        , const bool suppressConducts = false
        , const bool suppressEvents = false);

    bool generate(int seed);
    const Map<int, Map<int2, int> >& getDoors();
    const Map<int, Map<int2, int> >& getFloors();
    Pointer<Chest> getChestByContainerId(int characterId);
    Pointer<Conduct> getConductByCharacterId(int characterId);

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

    int16_t getFieldValue(FieldEnum field, int roomId) const;

    // Appends a logged event to the specified room's event log and to activation.request.eventLog.
    void addLoggedEvent(ActivationContext& activation, int roomId, LoggedEvent event);
    // Appends a logged event only to activation.request.eventLog (not the room log). Use for failure events.
    void addRequestLoggedEvent(ActivationContext& activation, LoggedEvent event);

    void pushTrigger(const iActivator* activator, int characterId, int targetId = -1, BehaviorEventEnum eventType = BEHAVIOR_EVENT_NIL);
    void runBehaviorTrigger(int agentId, const iActivator& wrapper, ConductEnum conductSlot, int observerCharacterId = -1, int actorCharacterId = -1);
    void tickNpcConducts();

    // Called by activators when they schedule an animation; advances the global animation clock.
    void setAnimationTime(const Timestamp& t);

    bool validateCharacterWithinRoom(int characterId, int roomId);
    bool validateDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);
    bool validateSharedDoorNotOccupied(int roomId, ChannelEnum channel, Cardinal dir);

private:
    void drainEventQueue(std::vector<PendingTrigger>& queue);

    // Pure turn-state transitions (Turner's old endTitanTurn/endBuilderTurn), with
    // no tick side effect. Static and match-agnostic so pathfinding's speculative
    // Match copies can advance turn state without constructing a MatchController
    // (and paying for its trait-computation setup) per candidate move.
    static CodeEnum advanceTitanTurnState(Match& match);
    // outTitanTurnEnded is set to true when this call also ended the titan's
    // (possibly phantom, solo-mode) turn.
    static CodeEnum advanceBuilderTurnState(Match& match, bool& outTitanTurnEnded);
};
