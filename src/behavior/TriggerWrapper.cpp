#include "TriggerWrapper.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEnum.hpp"
#include "Codeset.hpp"
#include "ConductMemory.hpp"
#include "ConductScanCharacters.hpp"
#include "DoorFlyweight.hpp"
#include "Inventory.hpp"
#include "ItemFlyweight.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Preactivation.hpp"
#include "RequestContext.hpp"
#include "TraitEnum.hpp"
#include <variant>

// ─── match evaluation ────────────────────────────────────────────────────────

static bool evaluateMatch(
    const TriggerMatch& m,
    ActivationContext&  activation,
    MatchController&    controller,
    Match&              match,
    int                 roomId,
    const Conduct&      conduct,
    int&                scannedCharacterId,
    int&                scannedObjectId,
    int&                /*scannedRoomId*/)
{
    return std::visit([&](auto&& cond) -> bool {
        using T = std::decay_t<decltype(cond)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchActor>) {
            const auto actorTraits = controller.getTraitsComputed(activation.character.characterId).final;
            return cond.match.matchesCharacter(actorTraits, activation.character.role)
                && cond.condition.passesCharacter(actorTraits);
        }
        else if constexpr (std::is_same_v<T, TriggerMatchTool>) {
            bool passed = false;
            activation.sourceItem.access([&](Item& item) {
                item.accessFlyweight([&](const ItemFlyweight& fw) {
                    passed = cond.match.matchesItem(fw.itemAttributes, item.type)
                          && cond.condition.passesCharacter(fw.itemAttributes);
                });
            });
            return passed;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchTarget>) {
            bool matched = false;
            bool found   = false;
            activation.targetCharacter().access([&](Character& target) {
                found = true;
                const auto targetTraits = controller.getTraitsComputed(target.characterId).final;
                matched = cond.match.matchesCharacter(targetTraits, target.role);
                if (matched && cond.match.locks.isAny()) {
                    bool lockMatched = false;
                    activation.targetChest().access([&](Chest& chest) {
                        LockFlyweight::getFlyweights().accessConst((int)chest.lock, [&](const LockFlyweight& lf) {
                            lockMatched = !(cond.match.locks - lf.lockAttributes).isAny();
                        });
                    });
                    matched = lockMatched;
                }
            });
            if (!found) {
                activation.targetItem().access([&](Item& item) {
                    found = true;
                    item.accessFlyweight([&](const ItemFlyweight& fw) {
                        matched = cond.match.matchesItem(fw.itemAttributes, item.type);
                    });
                });
            }
            if (!found) {
                activation.targetWall().access([&](Wall& w) {
                    DoorFlyweight::getFlyweights().accessConst((int)w.door, [&](const DoorFlyweight& fw) {
                        matched = cond.match.matchesDoor(fw.doorAttributes, w.door);
                    });
                });
            }
            return matched;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchReachedTargetRoom>) {
            const int targetRoom = conduct.get(cond.var);
            return targetRoom == -1
                || activation.character.location.roomId == targetRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchCharacter>) {
            const bool hasItemFilter = cond.itemAccepted[0] != ITEM_UNALLOCATED
                                    || cond.itemRejected[0] != ITEM_UNALLOCATED
                                    || cond.itemTraitsRequired.isAny()
                                    || cond.itemTraitsRestricted.isAny();
            const bool hasLockFilter = cond.lockTraitsRequired.isAny()
                                    || cond.lockTraitsRestricted.isAny()
                                    || cond.locksAccepted[0] != LOCK_COUNT
                                    || cond.locksRejected[0] != LOCK_COUNT;

            // Tool source: check the activation tool item against item filters only.
            if (cond.source == TriggerMatchActivationSource::Tool) {
                bool matched = false;
                activation.sourceItem.access([&](Item& item) {
                    if (!hasItemFilter) { matched = true; return; }
                    if (cond.itemAccepted[0] != ITEM_UNALLOCATED) {
                        bool found = false;
                        for (int i = 0; i < TriggerMatchCharacter::MAX_ITEM_FILTER && cond.itemAccepted[i] != ITEM_UNALLOCATED; i++)
                            if (item.type == cond.itemAccepted[i]) { found = true; break; }
                        if (!found) return;
                    }
                    if (cond.itemRejected[0] != ITEM_UNALLOCATED) {
                        for (int i = 0; i < TriggerMatchCharacter::MAX_ITEM_FILTER && cond.itemRejected[i] != ITEM_UNALLOCATED; i++)
                            if (item.type == cond.itemRejected[i]) return;
                    }
                    if (cond.itemTraitsRequired.isAny() || cond.itemTraitsRestricted.isAny()) {
                        bool traitPass = true;
                        item.accessFlyweight([&](const ItemFlyweight& fw) {
                            if (cond.itemTraitsRequired.isAny()   && (cond.itemTraitsRequired   - fw.itemAttributes).isAny()) traitPass = false;
                            if (cond.itemTraitsRestricted.isAny() && (cond.itemTraitsRestricted & fw.itemAttributes).isAny()) traitPass = false;
                        });
                        if (!traitPass) return;
                    }
                    matched = true;
                });
                return matched;
            }

            // Actor/Target source: check a character from the activation context.
            auto checkCharacter = [&](Character& ch) -> bool {
                const auto traits = controller.getTraitsComputed(ch.characterId).final;
                if (cond.traitsRequired.isAny()   && (cond.traitsRequired   - traits).isAny()) return false;
                if (cond.traitsRestricted.isAny() && (cond.traitsRestricted & traits).isAny()) return false;
                if (cond.rolesAccepted[0] != ROLE_COUNT) {
                    bool found = false;
                    for (int i = 0; i < TriggerMatchCharacter::MAX_ROLE_FILTER && cond.rolesAccepted[i] != ROLE_COUNT; i++)
                        if (ch.role == cond.rolesAccepted[i]) { found = true; break; }
                    if (!found) return false;
                }
                if (cond.rolesRejected[0] != ROLE_COUNT) {
                    for (int i = 0; i < TriggerMatchCharacter::MAX_ROLE_FILTER && cond.rolesRejected[i] != ROLE_COUNT; i++)
                        if (ch.role == cond.rolesRejected[i]) return false;
                }

                bool passes = true;
                if (hasItemFilter || hasLockFilter) {
                    activation.request.access([&](RequestContext& req) {
                        if (passes && hasItemFilter) {
                            bool hasMatchingItem = false;
                            CodeEnum itemErr = CODE_UNSET;
                            req.match.getCharacter(ch.characterId, itemErr).access([&](Character& c) {
                                const auto inv = c.getInventory(req.match.dungeon);
                                for (int i = 0; i < inv.size && !hasMatchingItem; i++) {
                                    const Item& item = inv.items[i];
                                    if (item.type == ITEM_UNALLOCATED || item.stacks == 0) continue;
                                    if (cond.itemAccepted[0] != ITEM_UNALLOCATED) {
                                        bool found = false;
                                        for (int j = 0; j < TriggerMatchCharacter::MAX_ITEM_FILTER && cond.itemAccepted[j] != ITEM_UNALLOCATED; j++)
                                            if (item.type == cond.itemAccepted[j]) { found = true; break; }
                                        if (!found) continue;
                                    }
                                    if (cond.itemRejected[0] != ITEM_UNALLOCATED) {
                                        bool blocked = false;
                                        for (int j = 0; j < TriggerMatchCharacter::MAX_ITEM_FILTER && cond.itemRejected[j] != ITEM_UNALLOCATED; j++)
                                            if (item.type == cond.itemRejected[j]) { blocked = true; break; }
                                        if (blocked) continue;
                                    }
                                    if (cond.itemTraitsRequired.isAny() || cond.itemTraitsRestricted.isAny()) {
                                        bool traitPass = true;
                                        item.accessFlyweight([&](const ItemFlyweight& fw) {
                                            if (cond.itemTraitsRequired.isAny()   && (cond.itemTraitsRequired   - fw.itemAttributes).isAny()) traitPass = false;
                                            if (cond.itemTraitsRestricted.isAny() && (cond.itemTraitsRestricted & fw.itemAttributes).isAny()) traitPass = false;
                                        });
                                        if (!traitPass) continue;
                                    }
                                    hasMatchingItem = true;
                                }
                            });
                            if (!hasMatchingItem) passes = false;
                        }
                        if (passes && hasLockFilter) {
                            bool lockPasses = false;
                            req.controller.getChestByContainerId(ch.characterId).access([&](Chest& chest) {
                                LockFlyweight::getFlyweights().accessConst((int)chest.lock, [&](const LockFlyweight& lf) {
                                    bool ok = true;
                                    if (cond.lockTraitsRequired.isAny()   && (cond.lockTraitsRequired   - lf.lockAttributes).isAny()) ok = false;
                                    if (cond.lockTraitsRestricted.isAny() && (cond.lockTraitsRestricted & lf.lockAttributes).isAny()) ok = false;
                                    if (ok && cond.locksAccepted[0] != LOCK_COUNT) {
                                        bool found = false;
                                        for (int i = 0; i < TriggerMatchCharacter::MAX_LOCK_ACCEPTED && cond.locksAccepted[i] != LOCK_COUNT; i++)
                                            if (chest.lock == cond.locksAccepted[i]) { found = true; break; }
                                        if (!found) ok = false;
                                    }
                                    if (ok && cond.locksRejected[0] != LOCK_COUNT) {
                                        for (int i = 0; i < TriggerMatchCharacter::MAX_LOCK_REJECTED && cond.locksRejected[i] != LOCK_COUNT; i++)
                                            if (chest.lock == cond.locksRejected[i]) { ok = false; break; }
                                    }
                                    lockPasses = ok;
                                });
                            });
                            if (!lockPasses) passes = false;
                        }
                    });
                }
                return passes;
            };

            if (cond.source == TriggerMatchActivationSource::Actor) {
                if (!checkCharacter(activation.character)) return false;
                scannedCharacterId = activation.character.characterId;
                return true;
            }
            if (cond.source == TriggerMatchActivationSource::Observer) {
                const int obsId = (activation.observerCharacterId != -1)
                                 ? activation.observerCharacterId
                                 : activation.character.characterId;
                if (obsId == activation.character.characterId) {
                    if (!checkCharacter(activation.character)) return false;
                    scannedCharacterId = obsId;
                    return true;
                }
                bool matched = false;
                for (auto& b : match.builders) {
                    if (b.character.characterId != obsId) continue;
                    if (checkCharacter(b.character)) { scannedCharacterId = obsId; matched = true; }
                    break;
                }
                if (!matched) {
                    for (auto& ch : match.dungeon.characters) {
                        if (ch.characterId != obsId) continue;
                        if (checkCharacter(ch)) { scannedCharacterId = obsId; matched = true; }
                        break;
                    }
                }
                return matched;
            }
            // Target source.
            bool matched = false;
            activation.targetCharacter().access([&](Character& ch) {
                if (checkCharacter(ch)) {
                    scannedCharacterId = ch.characterId;
                    matched = true;
                }
            });
            return matched;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchCharacterVisible>) {
            const int targetId = conduct.get(cond.var);
            if (targetId == -1) return false;
            bool inRoom = false;
            controller.isCharacterWithinRoom(targetId, roomId, inRoom);
            return inRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchCharacterLost>) {
            const int targetId = conduct.get(cond.var);
            if (targetId == -1) return true;
            bool inRoom = false;
            controller.isCharacterWithinRoom(targetId, roomId, inRoom);
            return !inRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchScanObject>) {
            for (auto& chest : match.dungeon.chests) {
                if (chest.containerCharacterId == -1) continue;
                bool inRoom = false;
                controller.isCharacterWithinRoom(chest.containerCharacterId, roomId, inRoom);
                if (!inRoom) continue;
                if (cond.match.locks.isAny()) {
                    bool lockMatched = false;
                    LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf) {
                        lockMatched = !(cond.match.locks - lf.lockAttributes).isAny();
                    });
                    if (!lockMatched) continue;
                }
                const auto traits = controller.getTraitsComputed(chest.containerCharacterId).final;
                RoleEnum role = ROLE_EMPTY;
                for (const auto& ch : match.dungeon.characters)
                    if (ch.characterId == chest.containerCharacterId) { role = ch.role; break; }
                if (!cond.match.matchesCharacter(traits, role)) continue;
                if (!cond.condition.passesCharacter(traits)) continue;
                scannedObjectId = chest.containerCharacterId;
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchObjectVisible>) {
            const int targetId = conduct.get(cond.var);
            if (targetId == -1) return false;
            bool inRoom = false;
            controller.isCharacterWithinRoom(targetId, roomId, inRoom);
            return inRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchCharacterId>) {
            const int expected = conduct.get(cond.var);
            if (expected == -1) return false;
            if (cond.source == TriggerMatchCharacterSource::Actor)
                return activation.character.characterId == expected;
            bool matched = false;
            activation.targetCharacter().access([&](Character& ch) {
                matched = (ch.characterId == expected);
            });
            return matched;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchObserver>) {
            const int obsId = (activation.observerCharacterId != -1)
                             ? activation.observerCharacterId
                             : activation.character.characterId;
            const auto obsTraits = controller.getTraitsComputed(obsId).final;
            RoleEnum obsRole = ROLE_EMPTY;
            for (const auto& b : match.builders)
                if (b.character.characterId == obsId) { obsRole = b.character.role; break; }
            if (obsRole == ROLE_EMPTY)
                for (const auto& ch : match.dungeon.characters)
                    if (ch.characterId == obsId) { obsRole = ch.role; break; }
            return cond.match.matchesCharacter(obsTraits, obsRole)
                && cond.condition.passesCharacter(obsTraits);
        }
        return false;
    }, m);
}

// ─── effect application ──────────────────────────────────────────────────────

static bool applyEffect(
    const TriggerEffect& e,
    ActivationContext&   activation,
    Conduct&             conduct,
    ConductMemory&       conductMem,
    int                  scannedCharacterId,
    int                  scannedObjectId,
    int                  scannedRoomId)
{
    return std::visit([&](auto&& eff) -> bool {
        using T = std::decay_t<decltype(eff)>;
        if constexpr (std::is_same_v<T, std::monostate>) { return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetBehavior>) {
            if (conductMem.state != eff.behavior) {
                conductMem.previousState = conductMem.state;
                activation.request.access([&](RequestContext& req) {
                    conductMem.stateChangedTurn = req.match.turner.turn;
                });
            }
            conductMem.state = eff.behavior;
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVar>)
            { conduct.set(eff.var, scannedCharacterId); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromObj>)
            { conduct.set(eff.var, scannedObjectId); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromRoom>)
            { conduct.set(eff.var, scannedRoomId); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromContext>)
            { activation.targetCharacter().access([&](Character& ch) { conduct.set(eff.var, ch.characterId); }); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromSelfRoom>)
            { conduct.set(eff.var, activation.character.location.roomId); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectClearVar>)
            { conduct.set(eff.var, -1); return true; }
        else if constexpr (std::is_same_v<T, TriggerEffectScanRoomForCharacter>) {
            bool found = false;
            activation.request.access([&](RequestContext& req) {
                const int actorRoomId = activation.character.location.roomId;
                auto tryCharacter = [&](int cid, RoleEnum role) -> bool {
                    if (cid == activation.character.characterId) return false;
                    bool inRoom = false;
                    req.controller.isCharacterWithinRoom(cid, actorRoomId, inRoom);
                    if (!inRoom) return false;
                    const auto traits = req.controller.getTraitsComputed(cid).final;
                    if (!eff.match.matchesCharacter(traits, role)) return false;
                    if (!eff.condition.passesCharacter(traits)) return false;
                    if (eff.match.locks.isAny()) {
                        bool lockMatched = false;
                        for (const auto& chest : req.match.dungeon.chests) {
                            if (chest.containerCharacterId != cid) continue;
                            LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf) {
                                lockMatched = !(eff.match.locks - lf.lockAttributes).isAny();
                            });
                            break;
                        }
                        if (!lockMatched) return false;
                    }
                    for (const TriggerInnerEffect& inner : eff.onFound) {
                        if (std::holds_alternative<std::monostate>(inner)) break;
                        applyInnerEffect(inner, activation, conduct, conductMem, cid, cid, scannedRoomId);
                    }
                    return true;
                };
                for (auto& builder : req.match.builders)
                    if (tryCharacter(builder.character.characterId, builder.character.role)) { found = true; return; }
                for (auto& ch : req.match.dungeon.characters)
                    if (tryCharacter(ch.characterId, ch.role)) { found = true; return; }
            });
            return found;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectScanCharacters>) {
            return applyScanCharacters(eff, activation, conduct);
        }
        else if constexpr (std::is_same_v<T, TriggerEffectComputePath>) {
            activation.request.access([&](RequestContext& req) {
                // Flag #3: suppress re-entrant A* when already inside findCharacterPath().
                if (req.controller.isPathfindingActive) return;

                const int pathCharId = (eff.characterVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(eff.characterVar)
                                        : activation.character.characterId;
                if (pathCharId == -1) return;

                const int targetRoom = (eff.targetRoomVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(eff.targetRoomVar) : -1;
                const int targetBits = (eff.targetBitsVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                        ? conduct.get(eff.targetBitsVar) : 0;
                const int loops = eff.loops != 0 ? eff.loops : TriggerEffectComputePath::DEFAULT_LOOPS;

                int seenBits      = 0;
                int traversedBits = 0;
                int doorBits[4]   = {};

                // Destination: character's simulated location matches target room or bits.
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

                // Heuristic: accumulate explored rooms; zero cost so this is Dijkstra.
                auto heuristic = [&seenBits](const CharacterAction& action, const Match&) -> int {
                    if (action.roomId >= 0 && action.roomId < 32)
                        seenBits |= (1 << action.roomId);
                    return 0;
                };

                // Consumer: record the optimal path's room bits and per-direction door bits.
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
                writeVar(eff.seenRoomBitsVar,      seenBits);
                writeVar(eff.traversedRoomBitsVar, traversedBits);
                writeVar(eff.doorNorthVar,         doorBits[Cardinal::north().getIndex()]);
                writeVar(eff.doorEastVar,          doorBits[Cardinal::east().getIndex()]);
                writeVar(eff.doorSouthVar,         doorBits[Cardinal::south().getIndex()]);
                writeVar(eff.doorWestVar,          doorBits[Cardinal::west().getIndex()]);
            });
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectComputeFloodPath>) {
            activation.request.access([&](RequestContext& req) {
                const int startRoom = (eff.startRoomVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                      ? conduct.get(eff.startRoomVar) : -1;
                const int startBits = (eff.startBitsVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                      ? conduct.get(eff.startBitsVar) : 0;

                if (startRoom == -1 && startBits == 0) return;

                int towardsBits[4] = {};
                int againstBits[4] = {};

                auto consumer = [&](const CharacterAction& action, const Match& result) {
                    if (action.roomId < 0 || action.roomId >= 32) return;
                    action.direction.accessConst([&](const Cardinal& dir) {
                        // Towards: source room gets a bit for the direction taken.
                        towardsBits[dir.getIndex()] |= (1 << action.roomId);

                        // Against: destination room gets a bit for the reversed direction.
                        const int destRoom = result.dungeon.pathfinderCharacter.location.roomId;
                        if (destRoom >= 0 && destRoom < 32)
                            againstBits[dir.getFlip().getIndex()] |= (1 << destRoom);
                    });
                };

                if (startRoom != -1)
                    req.controller.floodFillRoom(startRoom, consumer);
                else
                    req.controller.floodFillRoomBits(startBits, consumer);

                auto writeVar = [&](ConductMemoryVariableEnum var, int val) {
                    if (var != CONDUCT_MEMORY_VARIABLE_COUNT) conduct.set(var, val);
                };
                writeVar(eff.towardsNorthVar, towardsBits[Cardinal::north().getIndex()]);
                writeVar(eff.towardsEastVar,  towardsBits[Cardinal::east().getIndex()]);
                writeVar(eff.towardsSouthVar, towardsBits[Cardinal::south().getIndex()]);
                writeVar(eff.towardsWestVar,  towardsBits[Cardinal::west().getIndex()]);
                writeVar(eff.againstNorthVar,  againstBits[Cardinal::north().getIndex()]);
                writeVar(eff.againstEastVar,   againstBits[Cardinal::east().getIndex()]);
                writeVar(eff.againstSouthVar,  againstBits[Cardinal::south().getIndex()]);
                writeVar(eff.againstWestVar,   againstBits[Cardinal::west().getIndex()]);
            });
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetVarFromDirection>) {
            int dirIdx = -1;
            activation.direction.accessConst([&](const Cardinal& dir) { dirIdx = dir.getIndex(); });
            conduct.set(eff.var, dirIdx);
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetDirectionFromVar>) {
            const int dirIdx = conduct.get(eff.var);
            if (dirIdx >= 0 && dirIdx < 4)
                activation.direction = Cardinal(dirIdx);
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectExecuteAction>) {
            const Maybe<int> targetId   = (eff.targetVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                            ? Maybe<int>(conduct.get(eff.targetVar)) : Maybe<int>();
            const Maybe<int> sourceItem = (eff.toolVar != CONDUCT_MEMORY_VARIABLE_COUNT)
                                            ? Maybe<int>(conduct.get(eff.toolVar)) : Maybe<int>();
            activation.request.access([&](RequestContext& req) {
                req.controller.activate(Preactivation{
                    .action = {
                        .type              = eff.action,
                        .characterId       = activation.character.characterId,
                        .roomId            = activation.character.location.roomId,
                        .targetCharacterId = targetId,
                        .direction         = activation.direction,
                    },
                    .playerId             = req.player.account.toString(),
                    .sourceItemIndex      = sourceItem,
                    .isSkippingAnimations = req.isSkippingAnimations,
                });
            });
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectTraverseDoor>) {
            activation.request.access([&](RequestContext& req) {
                const int currentRoom = activation.character.location.roomId;
                if (currentRoom < 0 || currentRoom >= 32) return;
                const int roomBit = 1 << currentRoom;

                const ConductMemoryVariableEnum doorVars[4] = {
                    eff.doorNorthVar, eff.doorEastVar,
                    eff.doorSouthVar, eff.doorWestVar,
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
                    if (!req.controller.isFloorOccupied(adjacent, activation.character.location.channel, f, conflict)) {
                        freeFloor = f;
                        break;
                    }
                }
                if (freeFloor < 0) return;

                req.controller.activate(Preactivation{
                    .action = {
                        .type        = ACTION_MOVE_TO_FLOOR,
                        .characterId = activation.character.characterId,
                        .roomId      = adjacent,
                        .floorId     = Maybe<int>(freeFloor),
                    },
                    .playerId             = req.player.account.toString(),
                    .isSkippingAnimations = req.isSkippingAnimations,
                });
            });
            return true;
        }
        return true;
    }, e);
}

// ─── TriggerWrapper::activate ────────────────────────────────────────────────

bool TriggerWrapper::activate(ActivationContext& activation) const {
    bool fired = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& match      = req.match;
        auto& actor      = activation.character;

        const int conductOwnerId = (activation.observerCharacterId != -1)
                                 ? activation.observerCharacterId
                                 : actor.characterId;
        req.controller.getConductByCharacterId(conductOwnerId).access([&](Conduct& conduct) {

            const ConductEnum conductSlot = activation.conductSlot;
            const int roomId = activation.room.roomId;

            bool hasMemory = false;
            conduct.memory.accessConst(conductSlot, [&](const ConductMemory&) { hasMemory = true; });
            if (!hasMemory) return;

            int scannedCharacterId = -1;
            int scannedObjectId    = -1;
            int scannedRoomId      = -1;

            for (const TriggerMatch& m : _config.matches) {
                if (std::holds_alternative<std::monostate>(m)) break;
                if (!evaluateMatch(m, activation, controller, match, roomId,
                                conduct, scannedCharacterId,
                                scannedObjectId, scannedRoomId))
                    return;
            }

            conduct.memory.access(conductSlot, [&](ConductMemory& conductMem) {
                bool anyFailed = false;
                for (const TriggerEffect& e : _config.effects) {
                    if (std::holds_alternative<std::monostate>(e)) break;
                    if (!applyEffect(e, activation, conduct, conductMem,
                                     scannedCharacterId, scannedObjectId, scannedRoomId))
                        anyFailed = true;
                }
                conductMem.lastEffectCode = activation.codeset.error;
                if (anyFailed) {
                    for (const TriggerEffect& e : _config.failureEffects) {
                        if (std::holds_alternative<std::monostate>(e)) break;
                        applyEffect(e, activation, conduct, conductMem,
                                    scannedCharacterId, scannedObjectId, scannedRoomId);
                    }
                    conductMem.lastFailureEffectCode = activation.codeset.error;
                }
            });
        });

        fired = true;
    });
    return fired;
}
