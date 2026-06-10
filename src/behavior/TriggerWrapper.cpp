#include "TriggerWrapper.hpp"
#include "ActivationContext.hpp"
#include "BehaviorEnum.hpp"
#include "ConductMemory.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "RequestContext.hpp"
#include "TraitEnum.hpp"
#include <variant>

// ─── match evaluation ────────────────────────────────────────────────────────

static bool evaluateMatch(
    const TriggerMatch&  m,
    ActivationContext&   activation,
    MatchController&     controller,
    Match&               match,
    int                  roomId,
    const ConductMemory& memory,
    int&                 scannedCharacterId,
    int&                 scannedObjectId,
    int&                 /*scannedRoomId*/)
{
    return std::visit([&](auto&& cond) -> bool {
        using T = std::decay_t<decltype(cond)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return true;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchReachedTargetRoom>) {
            return memory.targetRoomId == -1
                || activation.character.location.roomId == memory.targetRoomId;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchPickpocketableCharacterVisible>) {
            for (auto& builder : match.builders) {
                const int cid = builder.character.characterId;
                if (cid == activation.character.characterId) continue;
                bool inRoom = false;
                controller.isCharacterWithinRoom(cid, roomId, inRoom);
                if (!inRoom) continue;
                const auto traits = controller.getTraitsComputed(cid).final;
                if (!traits[TRAIT_PICKPOCKETABLE].orElse(false)) continue;
                if (traits[TRAIT_ACTION_READY].orElse(false))    continue;
                scannedCharacterId = cid;
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchTargetCharacterVisible>) {
            if (memory.targetCharacterId == -1) return false;
            bool inRoom = false;
            controller.isCharacterWithinRoom(memory.targetCharacterId, roomId, inRoom);
            return inRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchTargetCharacterLost>) {
            if (memory.targetCharacterId == -1) return true;
            bool inRoom = false;
            controller.isCharacterWithinRoom(memory.targetCharacterId, roomId, inRoom);
            return !inRoom;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchUnlockedChestVisible>) {
            for (auto& chest : match.dungeon.chests) {
                if (chest.containerCharacterId == -1) continue;
                bool inRoom = false;
                controller.isCharacterWithinRoom(chest.containerCharacterId, roomId, inRoom);
                if (!inRoom) continue;
                bool isLocked = false;
                LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf) {
                    isLocked = lf.isLocked;
                });
                if (isLocked) continue;
                scannedObjectId = chest.containerCharacterId;
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, TriggerMatchTargetChestVisible>) {
            if (memory.targetObjectId == -1) return false;
            bool inRoom = false;
            controller.isCharacterWithinRoom(memory.targetObjectId, roomId, inRoom);
            return inRoom;
        }
        return false;
    }, m);
}

// ─── effect application ──────────────────────────────────────────────────────

static void applyEffect(
    const TriggerEffect& e,
    ActivationContext&   activation,
    ConductMemory&       memory,
    int                  scannedCharacterId,
    int                  scannedObjectId,
    int                  scannedRoomId)
{
    std::visit([&](auto&& eff) {
        using T = std::decay_t<decltype(eff)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            // empty slot
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetBehavior>) {
            memory.state = eff.behavior;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetTargetCharacter>) {
            memory.targetCharacterId = scannedCharacterId;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetTargetCharacterFromContext>) {
            activation.targetCharacter().access([&](Character& ch) {
                memory.targetCharacterId = ch.characterId;
            });
        }
        else if constexpr (std::is_same_v<T, TriggerEffectClearTargetCharacter>) {
            memory.targetCharacterId = -1;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetTargetObject>) {
            memory.targetObjectId = scannedObjectId;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectClearTargetObject>) {
            memory.targetObjectId = -1;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectSetTargetRoom>) {
            memory.targetRoomId = scannedRoomId;
        }
        else if constexpr (std::is_same_v<T, TriggerEffectClearTargetRoom>) {
            memory.targetRoomId = -1;
        }
    }, e);
}

// ─── TriggerWrapper::activate ────────────────────────────────────────────────

bool TriggerWrapper::activate(ActivationContext& activation) const {
    bool fired = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& match      = req.match;
        auto& actor      = activation.character;

        req.controller.getConductByCharacterId(actor.characterId).access([&](Conduct& conduct) {

            const int roomId = actor.location.roomId;

            // Snapshot conduct memory so that an effect on one field doesn't
            // influence match evaluation of a later match in the same call.
            ConductMemory snapshot;
            bool hasMemory = conduct.memory.accessConst(
                _config.conduct,
                [&](const ConductMemory& m) { snapshot = m; });
            if (!hasMemory) return;

            int scannedCharacterId = -1;
            int scannedObjectId    = -1;
            int scannedRoomId      = -1;

            for (const TriggerMatch& m : _config.matches) {
                if (std::holds_alternative<std::monostate>(m)) break;
                if (!evaluateMatch(m, activation, controller, match, roomId,
                                snapshot, scannedCharacterId,
                                scannedObjectId, scannedRoomId))
                    return;
            }

            conduct.memory.access(_config.conduct, [&](ConductMemory& memory) {
                for (const TriggerEffect& e : _config.effects) {
                    if (std::holds_alternative<std::monostate>(e)) break;
                    applyEffect(e, activation, memory,
                                scannedCharacterId, scannedObjectId, scannedRoomId);
                }
            });
        });

        fired = true;
    });
    return fired;
}
