#include "Conduct.hpp"
#include "ActionEnum.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorFlyweight.hpp"
#include "Cardinal.hpp"
#include "CharacterAction.hpp"
#include "Codeset.hpp"
#include "ConductMemory.hpp"
#include "ConductScanCharacters.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "ProposalEffect.hpp"
#include "ProposerConfig.hpp"
#include "RequestContext.hpp"
#include "RoleEnum.hpp"

#include <variant>

static void applyProposalEffect(const ProposalEffect& eff, Conduct& conduct, ConductMemory& conductMem, ActivationContext& ctx) {
    std::visit([&](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetBehavior>) {
            if (conductMem.state != e.behavior) {
                conductMem.previousState = conductMem.state;
                ctx.request.access([&](RequestContext& req) {
                    conductMem.stateChangedTurn = req.match.turner.turn;
                });
            }
            conductMem.state = e.behavior;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVar>) {
            conduct.set(e.var, -1);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromObj>) {
            conduct.set(e.var, -1);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromRoom>) {
            conduct.set(e.var, -1);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromContext>) {
            ctx.targetCharacter().access([&](Character& ch) { conduct.set(e.var, ch.characterId); });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromSelfRoom>) {
            conduct.set(e.var, ctx.character.location.roomId);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectClearVar>) {
            conduct.set(e.var, -1);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectScanRoomForCharacter>) {
            ctx.request.access([&](RequestContext& req) {
                const int actorRoomId = ctx.character.location.roomId;
                auto tryCharacter = [&](int cid, RoleEnum role) -> bool {
                    if (cid == ctx.character.characterId) return false;
                    bool inRoom = false;
                    req.controller.isCharacterWithinRoom(cid, actorRoomId, inRoom);
                    if (!inRoom) return false;
                    const auto traits = req.controller.getTraitsComputed(cid).final;
                    if (!e.match.matchesCharacter(traits, role)) return false;
                    if (!e.condition.passesCharacter(traits)) return false;
                    if (e.match.locks.isAny()) {
                        bool lockMatched = false;
                        for (const auto& chest : req.match.dungeon.chests) {
                            if (chest.containerCharacterId != cid) continue;
                            LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf) {
                                lockMatched = !(e.match.locks - lf.lockAttributes).isAny();
                            });
                            break;
                        }
                        if (!lockMatched) return false;
                    }
                    for (const TriggerInnerEffect& inner : e.onFound) {
                        if (std::holds_alternative<std::monostate>(inner)) break;
                        applyInnerEffect(inner, ctx, conduct, conductMem, cid, cid, -1);
                    }
                    return true;
                };
                for (auto& builder : req.match.builders)
                    if (tryCharacter(builder.character.characterId, builder.character.role)) return;
                for (auto& ch : req.match.dungeon.characters)
                    if (tryCharacter(ch.characterId, ch.role)) return;
            });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectScanCharacters>) {
            applyScanCharacters(e, ctx, conduct);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectComputePath>) {
            ctx.request.access([&](RequestContext& req) {
                if (req.controller.isPathfindingActive) return;

                const int pathCharId = (e.characterVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(e.characterVar)
                                        : ctx.character.characterId;
                if (pathCharId == -1) return;

                const int targetRoom = (e.targetRoomVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(e.targetRoomVar) : -1;
                const int targetBits = (e.targetBitsVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(e.targetBitsVar) : 0;
                const int loops = e.loops != 0 ? e.loops : TriggerEffectComputePath::DEFAULT_LOOPS;

                int seenBits      = 0;
                int traversedBits = 0;
                int doorBits[4]   = {};

                auto destination = [pathCharId, targetRoom, targetBits](const Match& m) -> bool {
                    int dummy = -1;
                    return m.findCharacter(dummy, [&](const Character& ch) -> bool {
                        if (ch.characterId != pathCharId) return false;
                        const int rid = ch.location.roomId;
                        if (targetRoom != -1 && rid == targetRoom) return true;
                        if (targetBits != 0 && rid >= 0 && rid < 32 && (targetBits & (1 << rid))) return true;
                        return false;
                    });
                };
                auto heuristic = [&seenBits](const CharacterAction& action, const Match&) -> int {
                    if (action.roomId >= 0 && action.roomId < 32)
                        seenBits |= (1 << action.roomId);
                    return 0;
                };
                auto consumer = [&traversedBits, &doorBits](const CharacterAction& action, const Match&) {
                    if (action.roomId >= 0 && action.roomId < 32) {
                        traversedBits |= (1 << action.roomId);
                        action.direction.accessConst([&](const Cardinal& dir) {
                            const int idx = dir.getIndex();
                            if (idx >= 0 && idx < 4)
                                doorBits[idx] |= (1 << action.roomId);
                        });
                    }
                };

                req.controller.findCharacterPath(
                    req.player.account.toString(),
                    pathCharId, loops,
                    destination, heuristic, consumer,
                    /*isFailure=*/false);

                auto writeVar = [&](ConductMemoryVariableEnum var, int val) {
                    if (var != CONDUCT_MEMORY_VARIABLE_COUNT) conduct.set(var, val);
                };
                writeVar(e.seenRoomBitsVar,      seenBits);
                writeVar(e.traversedRoomBitsVar, traversedBits);
                writeVar(e.doorNorthVar,         doorBits[Cardinal::north().getIndex()]);
                writeVar(e.doorEastVar,          doorBits[Cardinal::east().getIndex()]);
                writeVar(e.doorSouthVar,         doorBits[Cardinal::south().getIndex()]);
                writeVar(e.doorWestVar,          doorBits[Cardinal::west().getIndex()]);
            });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectComputeFloodPath>) {
            ctx.request.access([&](RequestContext& req) {
                const int startRoom = (e.startRoomVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                      ? conduct.get(e.startRoomVar) : -1;
                const int startBits = (e.startBitsVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                      ? conduct.get(e.startBitsVar) : 0;
                if (startRoom == -1 && startBits == 0) return;

                int towardsBits[4] = {};
                int againstBits[4] = {};

                auto consumer = [&](const CharacterAction& action, const Match& result) {
                    if (action.roomId < 0 || action.roomId >= 32) return;
                    action.direction.accessConst([&](const Cardinal& dir) {
                        towardsBits[dir.getIndex()] |= (1 << action.roomId);
                        const int destRoom = result.dungeon.pathfinderCharacter.location.roomId;
                        if (destRoom >= 0 && destRoom < 32)
                            againstBits[dir.getFlip().getIndex()] |= (1 << destRoom);
                    });
                };

                bool floodOk;
                if (startRoom != -1)
                    floodOk = req.controller.floodFillRoom(startRoom, consumer);
                else
                    floodOk = req.controller.floodFillRoomBits(startBits, consumer);

                auto writeVar = [&](ConductMemoryVariableEnum var, int val) {
                    if (var != CONDUCT_MEMORY_VARIABLE_COUNT) conduct.set(var, val);
                };
                writeVar(e.towardsNorthVar, towardsBits[Cardinal::north().getIndex()]);
                writeVar(e.towardsEastVar,  towardsBits[Cardinal::east().getIndex()]);
                writeVar(e.towardsSouthVar, towardsBits[Cardinal::south().getIndex()]);
                writeVar(e.towardsWestVar,  towardsBits[Cardinal::west().getIndex()]);
                writeVar(e.againstNorthVar,  againstBits[Cardinal::north().getIndex()]);
                writeVar(e.againstEastVar,   againstBits[Cardinal::east().getIndex()]);
                writeVar(e.againstSouthVar,  againstBits[Cardinal::south().getIndex()]);
                writeVar(e.againstWestVar,   againstBits[Cardinal::west().getIndex()]);
            });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectTraverseDoor>) {
            ctx.request.access([&](RequestContext& req) {
                const int currentRoom = ctx.character.location.roomId;
                if (currentRoom < 0 || currentRoom >= 32) return;
                const int roomBit = 1 << currentRoom;

                const ConductMemoryVariableEnum doorVars[4] = {
                    e.doorNorthVar, e.doorEastVar,
                    e.doorSouthVar, e.doorWestVar,
                };
                int dirIdx = -1;
                for (int i = 0; i < 4; i++) {
                    if (doorVars[i] == CONDUCT_MEMORY_VARIABLE_COUNT) continue;
                    if (conduct.get(doorVars[i]) & roomBit) { dirIdx = i; break; }
                }
                if (dirIdx == -1) return;

                const Cardinal dir = dirIdx == 0 ? Cardinal::north()
                                   : dirIdx == 1 ? Cardinal::east()
                                   : dirIdx == 2 ? Cardinal::south()
                                                 : Cardinal::west();

                int adjacent = -1;
                req.match.dungeon.rooms.accessConst(currentRoom, [&](const Room& room) {
                    adjacent = room.getWall(dir).adjacent;
                });
                if (adjacent < 0) return;

                int freeFloor = -1;
                int conflict  = -1;
                for (int f = 0; f < 32; f++) {
                    if (!req.controller.isFloorOccupied(adjacent, ctx.character.location.channel, f, conflict)) {
                        freeFloor = f;
                        break;
                    }
                }
                if (freeFloor < 0) return;

                req.controller.activate(Preactivation{
                    .action = {
                        .type        = ACTION_MOVE_TO_FLOOR,
                        .characterId = ctx.character.characterId,
                        .roomId      = adjacent,
                        .floorId     = Maybe<int>(freeFloor),
                    },
                    .playerId             = req.player.account.toString(),
                    .isSkippingAnimations = req.isSkippingAnimations,
                });
            });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromDirection>) {
            int dirIdx = -1;
            ctx.direction.accessConst([&](const Cardinal& dir) { dirIdx = dir.getIndex(); });
            conduct.set(e.var, dirIdx);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetDirectionFromVar>) {
            const int dirIdx = conduct.get(e.var);
            if (dirIdx >= 0 && dirIdx < 4)
                ctx.direction = Cardinal(dirIdx);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectExecuteAction>) {
            const Maybe<int> targetId   = (e.targetVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                            ? Maybe<int>(conduct.get(e.targetVar)) : Maybe<int>();
            const Maybe<int> sourceItem = (e.toolVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                            ? Maybe<int>(conduct.get(e.toolVar))   : Maybe<int>();
            ctx.request.access([&](RequestContext& req) {
                req.controller.activate(Preactivation{
                    .action = {
                        .type              = e.action,
                        .characterId       = ctx.character.characterId,
                        .roomId            = ctx.character.location.roomId,
                        .targetCharacterId = targetId,
                        .direction         = ctx.direction,
                    },
                    .playerId             = req.player.account.toString(),
                    .sourceItemIndex      = sourceItem,
                    .isSkippingAnimations = req.isSkippingAnimations,
                });
            });
        }
    }, eff);
}

bool Conduct::buildAndExecuteProposals(ActivationContext& ctx) {
    int bestPriority           = 0;
    bool hasBest               = false;
    const ProposerConfig* best = nullptr;
    ConductEnum bestSlot       = CONDUCT_NIL;

    for (int c = CONDUCT_NIL + 1; c < CONDUCT_COUNT; c++) {
        memory.accessConst(ConductEnum(c), [&](const ConductMemory& mem) {
            if (mem.state == BEHAVIOR_NIL) return;
            BehaviorFlyweight::getFlyweights().accessConst(mem.state, [&](const BehaviorFlyweight& fw) {
                fw.proposer.accessConst([&](const ProposerConfig& p) {
                    if (!hasBest || p.priority > bestPriority) {
                        bestPriority = p.priority;
                        best         = &p;
                        bestSlot     = ConductEnum(c);
                        hasBest      = true;
                    }
                });
            });
        });
    }

    if (!best) return false;

    memory.access(bestSlot, [&](ConductMemory& conductMem) {
        conductMem.latestPriority = best->priority;
        for (const ProposalEffect& eff : best->effects) {
            if (std::holds_alternative<std::monostate>(eff)) break;
            applyProposalEffect(eff, *this, conductMem, ctx);
        }
        conductMem.lastEffectCode = ctx.codeset.error;
    });
    return true;
}
