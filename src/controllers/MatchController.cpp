#include "ActionFlyweight.hpp"
#include "ActivationContext.hpp"
#include "CyclicalRack.hpp"
#include "BehaviorFlyweight.hpp"
#include "Player.hpp"
#include "TraitEnum.hpp"
#include "AssociatedPriorityMinQueue.hpp"
#include "ChannelEnum.hpp"
#include "CharacterAction.hpp"
#include "CharacterDigest.hpp"
#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int3.hpp"
#include "Inventory.hpp"
#include "ItemFlyweight.hpp"
#include "Keyframe.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "RequestContext.hpp"
#include "TargetEntity.hpp"
#include "TargetPreactivationEntity.hpp"
#include "RoleFlyweight.hpp"
#include "TraitModifier.hpp"
#include "StructuredLog.hpp"

MatchController::MatchController(Match& match, Codeset& codeset): match(match), codeset(codeset) {
    match.accessUsedCharacters([&](const Character& character) {
        traitsComputed.set(character.characterId, TraitModifier::computeCharacterTraits(character).orElse(TraitModifier::TraitComputation{}));
    });
    fields.recompute(match);
}

// functions
bool MatchController::activate(const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog) {
    bool isSuccess = false;
    ActionFlyweight::getFlyweights().accessConst((int)preactivation.action.type, [&](const ActionFlyweight& flyweight){
        codeset.addFailure(!flyweight.activator.accessConst([&](const iActivator& activator){
            isSuccess = activate(activator, preactivation, outEventLog);
        }), CODE_ACTION_MISSING_ACTIVATION);
    });
    return isSuccess;
}

bool MatchController::activate(const iActivator& activator, const Preactivation& preactivation, Pointer<std::vector<LoggedEvent>> outEventLog) {
    // TriggerEffectExecuteAction / TriggerEffectTraverseDoor re-enter activate() synchronously
    // from proposal effects, so a runaway conduct cascade (e.g. a move that keeps re-triggering
    // itself) recurses through this function rather than looping. Bail out once nesting gets
    // implausibly deep instead of overflowing the stack.
    if (activationDepth >= MAX_ACTIVATION_DEPTH) {
        codeset.addLog(CODE_ACTIVATION_DEPTH_LIMIT_EXCEEDED, activationDepth);
        codeset.addFailure(true, CODE_ACTIVATION_DEPTH_LIMIT_EXCEEDED);
        return false;
    }
    struct ActivationDepthGuard {
        MatchController& c;
        ~ActivationDepthGuard() { --c.activationDepth; }
    } depthGuard{*this};
    ++activationDepth;

    setupLocations();

    // Check if match has started
    if (codeset.addFailure(!match.isStarted(codeset.error))) return false;

    std::vector<PendingTrigger> eventQueue;
    eventQueuePtr = eventQueue;

    bool isSuccess = false;
    buildActivationContext(preactivation, [&](ActivationContext& ctx){
        codeset.addFailure(!(isSuccess = activator.activate(ctx)));
        outEventLog.access([&](std::vector<LoggedEvent>& eventLog) {
            eventLog.clear();
            ctx.request.access([&](RequestContext& request) {
                CyclicalRack<LoggedEvent> log(request.eventLog.begin(), 32, (size_t)request.eventLogHead, (size_t)request.eventLogSize);
                for (size_t i = 0; i < log.size(); ++i) {
                    eventLog.push_back(log[i]);
                }
            });
        });
    });
                

    if (!isSuccess) {
        eventQueuePtr = nullptr;
        return false;
    }

    drainEventQueue(eventQueue);
    eventQueuePtr = nullptr;
    return true;
}

bool MatchController::allocate(const CharacterAllocSpec& spec, const AttachmentContext& attachment, int& outCharacterId) {
    setupLocations(false);
    bool isSuccess = false;

    codeset.addFailure(!match.allocateCharacter([&](Character& character) {
        character.role = spec.role;
        match.allocateInventory(character);  // allocate early so chest spec can give items
        character.visibility = ~0x0;
        outCharacterId = character.characterId;

        if (attachment.type == AttachmentContext::Type::FLOOR) {
            int outFloorId = -1;
            if (codeset.addFailure(!findFreeFloor(attachment.roomId, attachment.channel, outFloorId)))
                return;
            character.location = Location::makeFloor(attachment.roomId, attachment.channel, outFloorId);
        }

        updateTraits(character);

        if (attachment.type == AttachmentContext::Type::FLOOR)
            character.location.apply(character.characterId, match.dungeon.rooms, floors, doors);

        // Conduct: allocate if role requires it or spec requests it
        auto applyDefaultConductStates = [&](Conduct& conduct, RoleEnum role) {
            RoleFlyweight::getFlyweights().accessConst(role, [&](const RoleFlyweight& fw) {
                for (int c = 0; c < CONDUCT_COUNT; c++) {
                    fw.defaultConductStates.accessConst(c, [&](const BehaviorEnum& state) {
                        if (state != BEHAVIOR_NIL)
                            conduct.memory.access(c, [&](ConductMemory& mem) { mem.state = state; });
                    });
                }
            });
        };

        bool needsConduct = spec.conduct.isPresent();
        if (!needsConduct) {
            RoleFlyweight::getFlyweights().accessConst(spec.role, [&](const RoleFlyweight& fw) {
                if (fw.traitsSourced.shares(makeTraitBits({TRAIT_CONDUCT})))
                    needsConduct = true;
            });
        }
        if (needsConduct) {
            if (codeset.addFailure(!match.allocateConduct([&](Conduct& conduct) {
                conduct.characterId = character.characterId;
                conductMap.set(character.characterId, Pointer<Conduct>(conduct));
                applyDefaultConductStates(conduct, spec.role);
            }), CODE_UNABLE_TO_FIND_FREE_CONDUCT_IN_DUNGEON))
                return;
        }

        // Chest: allocate if spec includes one
        bool chestSuccess = true;
        spec.chest.accessConst([&](const ChestAllocSpec& chestSpec) {
            const int cRoomId = attachment.roomId;
            const ChannelEnum cChannel = attachment.channel;
            chestSuccess = !codeset.addFailure(!match.allocateChest([&](Chest& chest) {
                chest.containerCharacterId = character.characterId;
                chest.lock = chestSpec.lock;
                chestContainerMap.set(character.characterId, Pointer<Chest>(chest));

                auto allocateInventoryCharacter = [&](const CharacterAllocSpec& cSpec, ItemEnum slotItem) {
                    int containedId = -1;
                    codeset.addFailure(!match.allocateCharacter([&](Character& contained) {
                        contained.role = cSpec.role;
                        contained.visibility = ~0x0;
                        containedId = contained.characterId;

                        bool cNeedsConduct = cSpec.conduct.isPresent();
                        if (!cNeedsConduct) {
                            RoleFlyweight::getFlyweights().accessConst(cSpec.role, [&](const RoleFlyweight& fw) {
                                if (fw.traitsSourced.shares(makeTraitBits({TRAIT_CONDUCT})))
                                    cNeedsConduct = true;
                            });
                        }
                        if (cNeedsConduct) {
                            codeset.addFailure(!match.allocateConduct([&](Conduct& conduct) {
                                conduct.characterId = contained.characterId;
                                conductMap.set(contained.characterId, Pointer<Conduct>(conduct));
                                applyDefaultConductStates(conduct, cSpec.role);
                            }), CODE_UNABLE_TO_FIND_FREE_CONDUCT_IN_DUNGEON);
                        }

                        updateTraits(contained);
                        contained.location = Location::makeChest(cRoomId, cChannel, character.characterId);
                    }), CODE_UNABLE_TO_FIND_FREE_CHARACTER_IN_DUNGEON);

                    if (containedId != -1) {
                        auto chestInv = character.getInventory(match.dungeon);
                        codeset.addFailure(!chestInv.giveItem(slotItem, codeset.error));
                        chestInv.accessItem(slotItem, [&](Item& item) { item.stacks = containedId; });
                    }
                };

                for (const auto& entry : chestSpec.inventory) {
                    if (const auto* e = std::get_if<ItemSpec>(&entry)) {
                        auto chestInv = character.getInventory(match.dungeon);
                        codeset.addFailure(!chestInv.giveItem(e->item, codeset.error));
                    } else if (const auto* e = std::get_if<CritterCharacterSpec>(&entry)) {
                        allocateInventoryCharacter(e->character, ITEM_CRITTER);
                    } else if (const auto* e = std::get_if<ContainedCharacterSpec>(&entry)) {
                        allocateInventoryCharacter(e->character, ITEM_CONTAINED);
                    }
                }
            }), CODE_UNABLE_TO_FIND_FREE_CHEST_IN_DUNGEON);
        });

        if (!chestSuccess) return;

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

bool MatchController::buildActivationContext(const Preactivation& preactivation, std::function<void(ActivationContext&)> consumer) {
    bool isSuccess = false;
    codeset.addFailure(!match.getPlayer(preactivation.playerId, codeset.error).access([&](Player& player) {
        codeset.addFailure(!match.getCharacter(preactivation.action.characterId, codeset.error).access([&](Character& character) {
            codeset.addFailure(!match.dungeon.getRoom(preactivation.action.roomId, codeset.error).access([&](Room& room) {
                RequestContext request{
                    .player = player,
                    .match = match,
                    .codeset = codeset,
                    .controller = *this,
                    .time = preactivation.time,
                    .floorId = preactivation.action.floorId,
                    .isSkippingAnimations = preactivation.isSkippingAnimations,
                    .isSkippingLogging = preactivation.isSortingState || preactivation.isSkippingLogging
                };
                ActivationContext activation{
                    .codeset = codeset,
                    .request = request,
                    .room = room,
                    .character = character,
                    .direction = preactivation.action.direction,
                    .isSortingState = preactivation.isSortingState
                };

                // Source item — absolute index into dungeon.items[]
                activation.sourceItem = match.dungeon.items.getPointer(preactivation.sourceItemIndex.orElse(-1));
                activation.targetItemIndex = preactivation.action.targetItemIndex;

                // Resolve TargetPreactivationEntity → TargetEntity
                std::visit([&](auto& t) {
                    using T = std::decay_t<decltype(t)>;
                    if constexpr (std::is_same_v<T, PreactivationTargetCharacter>) {
                        activation.targetEntity = match.getCharacter(t.characterId, codeset.error);
                    } else if constexpr (std::is_same_v<T, PreactivationTargetItem>) {
                        // t.itemIndex is absolute into dungeon.items[]
                        activation.targetEntity = match.dungeon.items.getPointer(t.itemIndex);
                    } else if constexpr (std::is_same_v<T, PreactivationTargetDoor>) {
                        activation.targetEntity = DoorTarget{ Pointer<Wall>(room.getWall(t.direction)) };
                    } else if constexpr (std::is_same_v<T, PreactivationTargetLock>) {
                        activation.targetEntity = LockTarget{ Pointer<Wall>(room.getWall(t.direction)) };
                    } else if constexpr (std::is_same_v<T, NoPreactivationTarget>) {
                        Cardinal dir;
                        if (preactivation.action.direction.copy(dir)) {
                            if (preactivation.action.type == ACTION_ACTIVATE_LOCK) {
                                activation.targetEntity = LockTarget{ Pointer<Wall>(room.getWall(dir)) };
                            } else if (preactivation.action.type == ACTION_ACTIVATE_DOOR
                                    || preactivation.action.type == ACTION_MOVE_TO_DOOR) {
                                activation.targetEntity = DoorTarget{ Pointer<Wall>(room.getWall(dir)) };
                            }
                        }
                        if (std::holds_alternative<NoTarget>(activation.targetEntity)) {
                            int charId;
                            if (preactivation.action.targetCharacterId.copy(charId)) {
                                activation.targetEntity = match.getCharacter(charId, codeset.error);
                            }
                        }
                    }
                }, preactivation.targetPreactivationEntity);
                consumer(activation);
                isSuccess = true;
            }));
        }));
    }));
    return isSuccess;
}

CodeEnum MatchController::advanceTitanTurnState(Match& match) {
    return match.turner.advanceTitanTurnState(match);
}

CodeEnum MatchController::advanceBuilderTurnState(Match& match, bool& outTitanTurnEnded) {
    return match.turner.advanceBuilderTurnState(match, outTitanTurnEnded);
}

bool MatchController::advanceTurnState(Match& match, const std::string& playerId, CodeEnum& error, bool& outTitanTurnEnded) {
    outTitanTurnEnded = false;
    return match.accessPlayer(playerId, error, [&](Titan&) {
        advanceTitanTurnState(match);
        outTitanTurnEnded = true;
    }, [&](Builder&) {
        advanceBuilderTurnState(match, outTitanTurnEnded);
    });
}

CodeEnum MatchController::endTitanTurn() {
    CodeEnum result = advanceTitanTurnState(match);
    if (result == CODE_SUCCESS) {
        match.turner.runNpcTurn(match, [&]{ tickNpcConducts(); });
        fields.recompute(match);
    }
    return result;
}

CodeEnum MatchController::endBuilderTurn() {
    bool titanTurnEnded = false;
    CodeEnum result = advanceBuilderTurnState(match, titanTurnEnded);
    if (result == CODE_SUCCESS && titanTurnEnded) {
        match.turner.runNpcTurn(match, [&]{ tickNpcConducts(); });
    }
    if (result == CODE_SUCCESS) {
        fields.recompute(match);
    }
    return result;
}

bool MatchController::endTurn(const std::string& playerId, CodeEnum& error){
    // TODO: handle end turn voting
    bool titanTurnEnded = false;
    if (!advanceTurnState(match, playerId, error, titanTurnEnded)) {
        return false;
    }
    if (titanTurnEnded) {
        match.turner.runNpcTurn(match, [&]{ tickNpcConducts(); });
    }
    fields.recompute(match);
    return true;
}

bool MatchController::findCharacterPath(
    const std::string& playerId,
    int characterId,
    int loops,
    std::function<bool(const Match&)> destination,
    std::function<int(const CharacterAction&, const Match&)> heuristic,
    std::function<void(const CharacterAction&, const Match&)> consumer,
    const bool isFailure,
    const bool suppressConducts,
    const bool suppressEvents
) {
    const bool prevConducts = suppressNpcConducts;
    const bool prevEvents   = suppressNpcEventResponse;
    isPathfindingActive  = true;
    if (suppressConducts) suppressNpcConducts      = true;
    if (suppressEvents)   suppressNpcEventResponse = true;
    struct PathfindingGuard {
        MatchController& c;
        bool prevConducts, prevEvents;
        ~PathfindingGuard() {
            c.isPathfindingActive  = false;
            c.suppressNpcConducts      = prevConducts;
            c.suppressNpcEventResponse = prevEvents;
        }
    } guard{*this, prevConducts, prevEvents};

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
        if (codeset.addFailure(frontier.isEmpty(), isFailure ? CODE_PATHFINDING_EXHAUSTED_ALL_POSSIBLITIES : CODE_PATHFINDING_EXHAUSTED_ALL_POSSIBLITIES_ISSUE)) {
            return false;
        }
        if (codeset.addFailure(loops-- <= 0, isFailure ? CODE_PATHFINDING_FAILED_DUE_TO_INSUFFICIENT_LOOPS : CODE_PATHFINDING_FAILED_DUE_TO_INSUFFICIENT_LOOPS_ISSUE)) {
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
                bool nextTitanTurnEnded = false; // discarded: pathfinding never ticks NPC conducts
                codeset.addFailure(!advanceTurnState(next, playerId, codeset.error, nextTitanTurnEnded), CODE_PATHFINDING_FAILED_TO_FORWARD_TURN);
                next.setPathfinding();
                int new_cost = cost_so_far.getOr(current, 0) + 1;

                if (!cost_so_far.containsKey(next) || new_cost < cost_so_far.getOr(next, 0)) {
                    cost_so_far.set(next, new_cost);
                    int priority = new_cost + heuristic(action, next);
                    frontier.push(priority, next);
                    // These sets ensure 'next' exists as a key in our maps
                    came_from.set(next, current);
                    action_from.set(next, action);
                }
            });
        });

        if (isFound) {
            // Path reconstruction: walk parent links back to `start`, collecting states by
            // value (came_from/action_from hand back copies), then replay forwards.
            std::vector<Match> path_sequence;
            Match step = goalMatch;

            while (!(step == start)) {
                path_sequence.push_back(step);
                Maybe<Match> parent = came_from.get(step);
                if (parent.isEmpty()) break;
                parent.copy(step);
            }

            // Execute in chronological order
            for (auto it = path_sequence.rbegin(); it != path_sequence.rend(); ++it) {
                const Match& m = *it;
                action_from.get(m).access([&](const CharacterAction& actionFrom){
                    consumer(actionFrom, m);
                });
            }

            return true;
        }
    }
    return false;
}

bool MatchController::floodFillRoomBits(
    int startRoomBits,
    std::function<void(const CharacterAction&, const Match&)> consumer
) {
    int pathfinderCharId;
    if (!match.containsCharacter(match.dungeon.pathfinderCharacter, pathfinderCharId))
        return false;

    std::string playerId;
    for (auto& builder : match.builders) {
        if (!builder.player.isEmpty()) {
            playerId = builder.player.account.toString();
            break;
        }
    }
    if (playerId.empty() && !match.titan.player.isEmpty())
        playerId = match.titan.player.account.toString();
    if (playerId.empty())
        return false;

    const bool prevConducts = suppressNpcConducts;
    const bool prevEvents   = suppressNpcEventResponse;
    suppressNpcConducts      = true;
    suppressNpcEventResponse = true;
    struct FloodGuard {
        MatchController& c; bool prevC, prevE;
        ~FloodGuard() { c.suppressNpcConducts = prevC; c.suppressNpcEventResponse = prevE; }
    } guard{*this, prevConducts, prevEvents};

    int visitedRooms = startRoomBits;
    Map<int3, bool> visitedLocations;
    std::vector<Match> frontier;

    // Enqueues a fresh exploration state from the floor of roomId using the
    // original dungeon layout (not an evolved BFS state), so each room's
    // connectivity is evaluated against the real game state.
    const auto enqueueRoom = [&](int roomId) {
        Match s = match;
        s.setPathfinding();
        Character& pf = s.dungeon.pathfinderCharacter;
        pf.role     = ROLE_PATHFINDER;
        pf.moves    = 0;
        pf.actions  = 0;
        pf.location = Location::makeFloor(roomId, CHANNEL_CORPOREAL, 0);
        visitedLocations.set(int3{roomId, (int)pf.location.type, pf.location.data}, true);
        frontier.push_back(s);
    };

    for (int i = 0; i < 32; i++) {
        if (startRoomBits & (1 << i))
            enqueueRoom(i);
    }

    while (!frontier.empty()) {
        Match current = frontier.front();
        frontier.erase(frontier.begin());

        const int currentRoom = current.dungeon.pathfinderCharacter.location.roomId;

        Codeset nullCodes;
        MatchController tempCtrl(current, nullCodes);
        tempCtrl.updateTraits(current.dungeon.pathfinderCharacter);
        tempCtrl.setupLocations(true);

        tempCtrl.permuteCharacterActions(playerId, pathfinderCharId,
            [&](const CharacterAction& action, const Match& result) {
                const Character& pf = result.dungeon.pathfinderCharacter;
                const int newRoom   = pf.location.roomId;
                const int3 locKey{newRoom, (int)pf.location.type, pf.location.data};

                if (visitedLocations.containsKey(locKey)) return;
                visitedLocations.set(locKey, true);

                if (newRoom != currentRoom && !(visitedRooms & (1 << newRoom))) {
                    // Pathfinder crossed into a previously unseen room.
                    // ACTION_MOVE_TO_FLOOR carries no direction; recover it from
                    // the pathfinder's pre-action door position so consumers can
                    // record per-direction bitmasks.
                    visitedRooms |= (1 << newRoom);
                    CharacterAction enriched = action;
                    const Location& preLoc = current.dungeon.pathfinderCharacter.location;
                    if (enriched.direction.isEmpty() && preLoc.type == LOCATION_DOOR) {
                        enriched.direction = Cardinal(preLoc.data);
                    }
                    consumer(enriched, result);
                    enqueueRoom(newRoom);
                } else if (newRoom == currentRoom) {
                    // Within-room transition (e.g. floor→door): continue exploring
                    // from this position so we can attempt door activation next.
                    Match nextState = result;
                    nextState.setPathfinding();
                    frontier.push_back(nextState);
                }
            }
        );
    }

    return true;
}

bool MatchController::floodFillRoom(
    int startRoomId,
    std::function<void(const CharacterAction&, const Match&)> consumer
) {
    return floodFillRoomBits(1 << startRoomId, consumer);
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

        // setup characterId
        match.accessAllCharacters([&](Character& character){
            codeset.addFailure(!match.containsCharacter(character, character.characterId), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_CHARACTER);
        });

        // setup roomId
        for (Room& room: match.dungeon.rooms) {
            codeset.addFailure(!match.dungeon.containsRoom(room, room.roomId), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_ROOM);
        }

        // generate match (applies generator + full remodel list)
        codeset.addFailure(!(success = flyweight.buildMatch(seed, match, codeset)));
    }), CODE_MATCH_GENERATE_FAILED_DUE_TO_INNACCESSIBLE_GENERATOR_FLYWEIGHT);

    // Rebuild traitsComputed from the fully-populated match so callers on this
    // controller instance see up-to-date traits (the inner generator creates its
    // own MatchController whose traits map is discarded on return).
    if (success) {
        match.accessUsedCharacters([&](const Character& character) {
            traitsComputed.set(character.characterId, TraitModifier::computeCharacterTraits(character).orElse(TraitModifier::TraitComputation{}));
        });
    }

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

Pointer<Chest> MatchController::getChestByContainerId(int characterId) {
    setupLocations(false);
    return chestContainerMap.getOr(characterId, Pointer<Chest>::empty());
}

Pointer<Conduct> MatchController::getConductByCharacterId(int characterId) {
    setupLocations(false);
    return conductMap.getOr(characterId, Pointer<Conduct>::empty());
}

bool MatchController::giveInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun) {
    return !codeset.addFailure(!inventory.giveItem(type, codeset.error, isDryrun));
}

bool MatchController::giveInventoryItem(Inventory& inventory, const ItemEnum type, const Timestamp& time, const int roomId, const bool isSkippingAnimations) {
    if (!giveInventoryItem(inventory, type)) return false;
    if (!isSkippingAnimations) {
        inventory.accessItem(type, [&](Item& item) {
            Keyframe::insertKeyframe(
                Rack<Keyframe>::buildFromArray<Item::MAX_KEYFRAMES>(item.keyframes),
                Keyframe::buildTransition(time, 300, roomId, ANIMATION_FALL, ITEM_NIL, type)
            );
        });
    }
    return true;
}

bool MatchController::isCharacterActorValidation(const Character& character, const bool isCheckingCount) {
    return !codeset.addFailure(!character.isActor(codeset.error, getTraitsComputed(character.characterId).final, isCheckingCount));
}

bool MatchController::isCharacterMoverValidation(const Character& character, const bool isCheckingCount) {
    return !codeset.addFailure(!character.isMovable(codeset.error, getTraitsComputed(character.characterId).final, isCheckingCount));
}

bool MatchController::isCharacterKeyerValidation(const Character& character) {
    return !codeset.addFailure(!character.isKeyer(codeset.error, getTraitsComputed(character.characterId).final));
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
    doors.get(roomId).access([&](const Map<int2, int>& mapping){
        outCharacterId = mapping.getOr(int2{channel, dir.getIndex()}, -1);
    });
    return match.containsOffset(outCharacterId);
}

bool MatchController::isDoorOccupied(int roomId, ChannelEnum channel, Cardinal dir, int& outCharacterId) {
    return isDoorOccupied(roomId, channel, dir, outCharacterId, match, doors);
}

bool MatchController::isFloorOccupied(int roomId, ChannelEnum channel, int floorId, int& outCharacterId, const Match& match, const Map<int, Map<int2, int> >& floors) {
    outCharacterId = -1;
    floors.get(roomId).access([&](const Map<int2, int>& mapping){
        outCharacterId = mapping.getOr(int2{channel, floorId}, -1);
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
                            .action = action,
                            .playerId = playerId,
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
            if (codeset.addFailure(!character.getDigest(codeset.error, digest, getTraitsComputed(character.characterId).final))) {
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
                        case ROLE_TOGGLER_BLUE:
                        case ROLE_TOGGLER_ORANGE:
                            action.characterId = targetCharacterId;
                            action.targetCharacterId = mainCharacterId;
                            return applyAction(false, action);
                    }
                });
            };

            // character actions
            if (digest.actionsRemaining.orElse(0) > 0) {
                
                const auto activateOccupants = [&](const Map<int, Map<int2, int> >& cells) {
                    cells.get(roomId).access([&](const Map<int2, int>& cellMap) {
                        cellMap.forEach([&](const int2&, const int& offset) {
                            processTargetCharacter(CharacterAction{ .type = ACTION_ACTIVATE_CHARACTER, .characterId = mainCharacterId, .roomId = roomId }, offset);
                        });
                    });
                };
                activateOccupants(floors);
                activateOccupants(doors);

                // chest interactions
                for (Chest& chest : match.dungeon.chests) {
                    if (chest.containerCharacterId == -1) continue;
                    bool containerInRoom = false;
                    isCharacterWithinRoom(chest.containerCharacterId, roomId, containerInRoom);
                    if (!containerInRoom) continue;
                    bool isLocked = false;
                    LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf){
                        isLocked = lf.isLocked;
                    });
                    if (isLocked) {
                        applyAction(false, CharacterAction{
                            .type = ACTION_USE_CHEST_LOCK,
                            .characterId = mainCharacterId,
                            .roomId = roomId,
                            .targetCharacterId = chest.containerCharacterId,
                        });
                    } else {
                        match.getCharacter(chest.containerCharacterId, codeset.error).access([&](Character& containerChar) {
                            Inventory chestInv = containerChar.getInventory(match.dungeon);
                            for (int slot = 0; slot < chestInv.size; slot++) {
                                const Item& item = chestInv.items[slot];
                                bool isTransferable = false;
                                item.accessFlyweight([&](const ItemFlyweight& flyweight) {
                                    isTransferable = flyweight.itemAttributes[TRAIT_ITEM_TRANSFERABLE].orElse(false);
                                });
                                if (isTransferable) {
                                    applyAction(false, CharacterAction{
                                        .type = ACTION_LOOT_CHEST,
                                        .characterId = mainCharacterId,
                                        .roomId = roomId,
                                        .targetCharacterId = chest.containerCharacterId,
                                        .targetItemIndex = containerChar.itemStartIndex + slot,
                                    });
                                }
                            }
                        });
                    }
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
    chestContainerMap.clear();
    conductMap.clear();
    match.accessUsedCharacters([&](const Character& character){
        int characterId = -1;
        match.containsCharacter(character, characterId);
        character.location.apply(characterId, match.dungeon.rooms, floors, doors);
    });

    for (Chest& chest : match.dungeon.chests) {
        if (chest.containerCharacterId != -1) {
            chestContainerMap.set(chest.containerCharacterId, Pointer<Chest>(chest));
        }
    }

    for (Conduct& conduct : match.dungeon.conducts) {
        if (conduct.characterId != -1) {
            conductMap.set(conduct.characterId, Pointer<Conduct>(conduct));
        }
    }

    isLocationsSetup = true;
}

void MatchController::sortFloorCharacters(int roomId) {
    
}

bool MatchController::takeCharacterAction(Character& character) {
    if (!character.takeAction(codeset.error)) {
        codeset.addError(codeset.error);
        return false;
    }
    updateTraits(character);
    return true;
}

bool MatchController::takeCharacterMove(Character& character) {
    if (!character.takeMove(codeset.error)) {
        codeset.addError(codeset.error);
        return false;
    }
    updateTraits(character);
    return true;
}

bool MatchController::giveCharacterAction(Character& character) {
    character.actions = std::max(0, character.actions - 1);
    updateTraits(character);
    return true;
}

bool MatchController::giveCharacterMove(Character& character) {
    character.moves = std::max(0, character.moves - 1);
    updateTraits(character);
    return true;
}

bool MatchController::breakArmorItem(Character& character) {
    for (auto& builder : match.builders) {
        if (builder.character.characterId != character.characterId) continue;
        auto inv = builder.player.getInventory(match.dungeon);
        inv.takeItem(ITEM_ARMOR, codeset.error);
        return true;
    }
    return true;
}

void MatchController::addLoggedEvent(ActivationContext& activation, int roomId, LoggedEvent event) {
    activation.request.access([&](RequestContext& request) {
        if (request.isSkippingLogging) return;
        request.match.dungeon.rooms.access(roomId, [&](Room& room) {
            auto log = room.getEventLog();
            log.push_back(event);
            room.loggedHead = (int)log.getHeadOffset();
            room.loggedSize = (int)log.size();
        });
        CyclicalRack<LoggedEvent> reqLog(request.eventLog.begin(), 32, (size_t)request.eventLogHead, (size_t)request.eventLogSize);
        reqLog.push_back(event);
        request.eventLogHead = (int)reqLog.getHeadOffset();
        request.eventLogSize = (int)reqLog.size();
    });
}

void MatchController::addRequestLoggedEvent(ActivationContext& activation, LoggedEvent event) {
    activation.request.access([&](RequestContext& request) {
        if (request.isSkippingLogging) return;
        CyclicalRack<LoggedEvent> reqLog(request.eventLog.begin(), 32, (size_t)request.eventLogHead, (size_t)request.eventLogSize);
        reqLog.push_back(event);
        request.eventLogHead = (int)reqLog.getHeadOffset();
        request.eventLogSize = (int)reqLog.size();
    });
}

void MatchController::runBehaviorTrigger(int agentId, const iActivator& wrapper, ConductEnum conductSlot, int observerCharacterId, int actorCharacterId) {
    Player* playerProxy = nullptr;
    for (auto& builder : match.builders) {
        playerProxy = &builder.player;
        break;
    }
    if (!playerProxy) return;

    match.getCharacter(agentId, codeset.error).access([&](Character& agentChar) {
        match.dungeon.getRoom(agentChar.location.roomId, codeset.error).access([&](Room& room) {
            RequestContext request{
                .player = *playerProxy,
                .match = match,
                .codeset = codeset,
                .controller = *this,
                .time = animationTime,
                .isSkippingAnimations = false,
                .isSkippingLogging = true,
            };

            // In observer triggers, activation.character is the original event actor so that
            // TriggerMatchCharacter{source=Actor} checks the character who caused the event,
            // not the observing NPC. Proposals always run as the agent (monkey) after.
            const bool useActorChar = (observerCharacterId != -1)
                                    && (actorCharacterId != -1)
                                    && (actorCharacterId != agentId);

            auto fireAndPropose = [&](Character& actorChar) {
                {
                    ActivationContext activation{
                        .codeset = codeset,
                        .request = request,
                        .room = room,
                        .character = actorChar,
                        .conductSlot = conductSlot,
                        .observerCharacterId = observerCharacterId,
                    };
                    wrapper.activate(activation);
                }
                // After the trigger may have changed FSM state, run proposals for the
                // new state immediately (e.g. PICKPOCKET_ATTEMPT → steal, STASH_DEPOSITING → deposit).
                ActivationContext proposalCtx{
                    .codeset = codeset,
                    .request = request,
                    .room = room,
                    .character = agentChar,
                    .conductSlot = conductSlot,
                    .observerCharacterId = observerCharacterId,
                };
                getConductByCharacterId(agentId).access([&](Conduct& conduct) {
                    conduct.buildAndExecuteProposals(proposalCtx);
                });
            };

            if (useActorChar) {
                match.getCharacter(actorCharacterId, codeset.error).access([&](Character& actorChar) {
                    fireAndPropose(actorChar);
                });
            } else {
                ActivationContext activation{
                    .codeset = codeset,
                    .request = request,
                    .room = room,
                    .character = agentChar,
                    .conductSlot = conductSlot,
                    .observerCharacterId = observerCharacterId,
                };
                wrapper.activate(activation);
                getConductByCharacterId(agentId).access([&](Conduct& conduct) {
                    conduct.buildAndExecuteProposals(activation);
                });
            }
        });
    });
}

void MatchController::pushTrigger(const iActivator* activator, int characterId, int targetId, BehaviorEventEnum eventType) {
    eventQueuePtr.access([&](std::vector<PendingTrigger>& q) {
        q.push_back(PendingTrigger{ activator, characterId, targetId, Maybe<int>::empty(), eventType });
    });
}

void MatchController::drainEventQueue(std::vector<PendingTrigger>& eventQueue) {
    constexpr int MAX_QUEUE_SIZE = 200;
    constexpr int MAX_PROCESSED  = 400;
    int totalProcessed = 0;
    while (!eventQueue.empty()) {
        if ((int)eventQueue.size() > MAX_QUEUE_SIZE) {
            codeset.addLog(CODE_EVENT_QUEUE_SIZE_LIMIT_EXCEEDED, (int)eventQueue.size());
            break;
        }
        if (totalProcessed >= MAX_PROCESSED) {
            codeset.addLog(CODE_EVENT_QUEUE_PROCESSED_LIMIT_EXCEEDED, totalProcessed);
            break;
        }

        auto trigger = eventQueue.front();
        eventQueue.erase(eventQueue.begin());
        ++totalProcessed;

        // TODO: run trigger.activator in a full ActivationContext context with time = animationTime

        // Behavior responses are not cascaded (BEHAVIOR_EVENT_NIL suppresses further dispatch).
        if (trigger.eventType == BEHAVIOR_EVENT_NIL) continue;

        int roomId = -1;
        match.accessUsedCharacters([&](const Character& ch) {
            if (ch.characterId == trigger.characterId) roomId = ch.location.roomId;
        });
        if (roomId < 0) continue;

        if (suppressNpcEventResponse) continue;
        setupLocations();

        // Nested-map case (see Map::forEach's note): runBehaviorTrigger() below can relocate a
        // character - even clear() and rebuild floors - so snapshot the room's occupant ids
        // into a local. floors.get() already hands back a detached copy of the inner map.
        std::vector<int> roomAgentIds;
        floors.get(roomId).access([&](const Map<int2, int>& floorMap) {
            floorMap.forEach([&](const int2&, const int& agentId) { roomAgentIds.push_back(agentId); });
        });
        slog::emit(slog::Level::Debug, "drainEventQueue", "drain:dispatch",
                   {{"trigger_char", trigger.characterId}, {"room_id", roomId},
                    {"event_type", (int)trigger.eventType}, {"agent_count", (int)roomAgentIds.size()}});

        for (int agentId : roomAgentIds) {
            getConductByCharacterId(agentId).access([&](Conduct& conduct) {
                for (int c = CONDUCT_NIL + 1; c < CONDUCT_COUNT; c++) {
                    conduct.memory.accessConst(ConductEnum(c), [&](const ConductMemory& mem) {
                        if (mem.state == BEHAVIOR_NIL) return;
                        BehaviorFlyweight::getFlyweights().accessConst(mem.state, [&](const BehaviorFlyweight& fw) {
                            const BehaviorFlyweight::EventTriggers* triggers = fw.getTriggersForEvent(trigger.eventType);
                            if (!triggers) return;
                            const bool isObserver = (agentId != trigger.characterId && agentId != trigger.targetId);
                            const Maybe<TriggerWrapper>& slot = (agentId == trigger.characterId)
                                ? triggers->asActor
                                : (agentId == trigger.targetId)
                                    ? triggers->asTarget
                                    : triggers->asObserver;
                            slot.accessConst([&](const TriggerWrapper& wrapper) {
                                runBehaviorTrigger(agentId, wrapper, ConductEnum(c), isObserver ? agentId : -1, trigger.characterId);
                            });
                        });
                    });
                }
            });
        }
    }
}

void MatchController::tickNpcConducts() {
    if (suppressNpcConducts) return;
    if (!match.isStarted(codeset.error)) return;
    setupLocations();

    Player* playerProxy = nullptr;
    for (auto& builder : match.builders) {
        playerProxy = &builder.player;
        break;
    }
    if (!playerProxy) return;

    std::vector<PendingTrigger> eventQueue;
    eventQueuePtr = eventQueue;

    // buildAndExecuteProposals()/drainEventQueue() below can relocate characters and rebuild
    // conductMap from inside the loop, so iterate it reentrancy-safely (see Map::forEach).
    // The breadcrumbs are DEBUG, sync-flushed under X-Sync-Log: a crash between two of them
    // names the conduct and sub-phase it died in.
    slog::emit(slog::Level::Debug, "tickNpcConducts", "npc_tick:loop_begin",
               {{"conduct_count", (int)conductMap.size()}});

    conductMap.forEach([&](int charId, const Pointer<Conduct>& conductSlot) {
        slog::emit(slog::Level::Debug, "tickNpcConducts", "npc_tick:conduct_begin", {{"char_id", charId}});
        conductSlot.access([&](Conduct& conduct) {
            match.getCharacter(charId, codeset.error).access([&](Character& character) {
                match.dungeon.getRoom(character.location.roomId, codeset.error).access([&](Room& room) {
                    RequestContext request{
                        .player = *playerProxy,
                        .match = match,
                        .codeset = codeset,
                        .controller = *this,
                        .time = animationTime,
                        .isSkippingAnimations = false,
                        .isSkippingLogging = true,
                    };
                    ActivationContext activation{
                        .codeset = codeset,
                        .request = request,
                        .room = room,
                        .character = character,
                    };
                    conduct.buildAndExecuteProposals(activation);
                });
            });
        });
        slog::emit(slog::Level::Debug, "tickNpcConducts", "npc_tick:drain_begin", {{"char_id", charId}});
        drainEventQueue(eventQueue);
        slog::emit(slog::Level::Debug, "tickNpcConducts", "npc_tick:conduct_done", {{"char_id", charId}});
    });

    eventQueuePtr = nullptr;
}

void MatchController::setAnimationTime(const Timestamp& t) {
    if (t > animationTime) {
        animationTime = t;
    }
}


bool MatchController::takeInventoryItem(Inventory& inventory, const ItemEnum type, const bool isDryrun) {
    return !codeset.addFailure(!inventory.takeItem(type, codeset.error, isDryrun));
}

bool MatchController::takeInventoryItem(Inventory& inventory, const ItemEnum type, const Timestamp& time, const int roomId, const bool isSkippingAnimations) {
    if (!isSkippingAnimations) {
        inventory.accessItem(type, [&](Item& item) {
            Keyframe::insertKeyframe(
                Rack<Keyframe>::buildFromArray<Item::MAX_KEYFRAMES>(item.keyframes),
                Keyframe::buildTransition(time, 300, roomId, ANIMATION_RISE, type, ITEM_NIL)
            );
        });
    }
    return takeInventoryItem(inventory, type);
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

void MatchController::updateTraits(Character& character) {
    traitsComputed.set(character.characterId, TraitModifier::computeCharacterTraits(character).orElse(TraitModifier::TraitComputation{}));
}

TraitModifier::TraitComputation MatchController::getTraitsComputed(int characterId) const {
    return traitsComputed.getOr(characterId, TraitModifier::TraitComputation{});
}

const Map<int, TraitModifier::TraitComputation>& MatchController::getTraitsComputedMap() const {
    return traitsComputed;
}

int16_t MatchController::getFieldValue(FieldEnum field, int roomId) const {
    return fields.get(field, roomId);
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
