#include "ActivatorBehaviorActPickpocketWander.hpp"
#include "ActivationContext.hpp"
#include "ActionEnum.hpp"
#include "Codeset.hpp"
#include "ConductEnum.hpp"
#include "ConductMemory.hpp"
#include "EventFlyweight.hpp"
#include "Inventory.hpp"
#include "ItemEnum.hpp"
#include "Location.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"
#include "Player.hpp"
#include "Preactivation.hpp"
#include "TraitEnum.hpp"

bool ActivatorBehaviorActPickpocketWander::activate(ActivationContext& activation) const {
    bool result = false;
    activation.request.access([&](RequestContext& req) {
        auto& controller = req.controller;
        auto& match      = req.match;
        auto& actor      = activation.character;
        const int roomId = actor.location.roomId;

        const auto traits = controller.getTraitsComputed(actor.characterId).final;

        // Score each eligible conduct; pick the highest.
        ConductEnum bestConduct = CONDUCT_NIL;
        int bestScore = 0;

        auto consider = [&](ConductEnum c, int score) {
            if (score > bestScore) { bestConduct = c; bestScore = score; }
        };

        // CONDUCT_DEPOSIT — finish squirreling a stolen item (highest priority).
        if (traits[TRAIT_ITEM_SQUIRRELER].orElse(false) && traits[TRAIT_ACTION_READY].orElse(false)) {
            actor.conduct.memory.access(CONDUCT_DEPOSIT, [&](const ConductMemory& mem) {
                if (mem.targetObjectId == -1) return;
                for (const auto& chest : match.dungeon.chests) {
                    if (chest.containerCharacterId == -1) continue;
                    bool inRoom = false;
                    controller.isCharacterWithinRoom(chest.containerCharacterId, roomId, inRoom);
                    if (!inRoom) continue;
                    bool isLocked = false;
                    LockFlyweight::getFlyweights().accessConst(chest.lock, [&](const LockFlyweight& lf) {
                        isLocked = lf.isLocked;
                    });
                    if (!isLocked) { consider(CONDUCT_DEPOSIT, 3); break; }
                }
            });
        }

        // CONDUCT_PICKPOCKET — steal from a victim in the same room.
        if (traits[TRAIT_PICKPOCKETER].orElse(false) && traits[TRAIT_ACTION_READY].orElse(false)) {
            actor.conduct.memory.access(CONDUCT_PICKPOCKET, [&](ConductMemory& mem) {
                mem.targetCharacterId = -1;
                for (auto& builder : match.builders) {
                    bool inRoom = false;
                    controller.isCharacterWithinRoom(builder.character.characterId, roomId, inRoom);
                    if (!inRoom) continue;
                    const auto vTraits = controller.getTraitsComputed(builder.character.characterId).final;
                    if (!vTraits[TRAIT_PICKPOCKETABLE].orElse(false)) continue;
                    if (vTraits[TRAIT_ACTION_READY].orElse(false)) continue;
                    mem.targetCharacterId = builder.character.characterId;
                    break;
                }
                if (mem.targetCharacterId != -1)
                    consider(CONDUCT_PICKPOCKET, 2);
            });
        }

        // CONDUCT_WANDER — move to a random free floor (lowest priority).
        if (traits[TRAIT_WANDERER].orElse(false) && traits[TRAIT_MOVEMENT_READY].orElse(false))
            consider(CONDUCT_WANDER, 1);

        // Activate the winning conduct.
        switch (bestConduct) {

            case CONDUCT_DEPOSIT: {
                actor.conduct.memory.access(CONDUCT_DEPOSIT, [&](ConductMemory& mem) {
                    const auto stolenItem = (ItemEnum)mem.targetObjectId;
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
                        if (!controller.takeCharacterAction(actor)) break;
                        CodeEnum chestErr = CODE_UNKNOWN_ERROR;
                        Inventory chestInv{};
                        match.getCharacter(chest.containerCharacterId, chestErr).access([&](Character& c) {
                            chestInv = c.getInventory(match.dungeon);
                        });
                        if (controller.giveInventoryItem(chestInv, stolenItem)) {
                            mem.targetObjectId = -1;
                            controller.updateTraits(actor);
                            result = true;
                            controller.addLoggedEvent(activation, roomId, LoggedEvent{
                                EVENT_DEPOSIT_LOOT,
                                { EventComponentKind::ROLE, (int)actor.role },
                                {},
                                { EventComponentKind::ITEM, (int)stolenItem },
                                -1
                            });
                        }
                        break;
                    }
                });
                break;
            }

            case CONDUCT_PICKPOCKET: {
                actor.conduct.memory.access(CONDUCT_PICKPOCKET, [&](const ConductMemory& mem) {
                    result = controller.activate(Preactivation{
                        .action = {
                            .type              = ACTION_PICKPOCKET,
                            .characterId       = actor.characterId,
                            .roomId            = roomId,
                            .targetCharacterId = mem.targetCharacterId,
                        },
                        .playerId             = req.player.account.toString(),
                        .isSkippingAnimations = req.isSkippingAnimations,
                    });
                });
                break;
            }

            case CONDUCT_WANDER: {
                int freeFloorId = -1;
                if (!controller.findFreeFloor(roomId, actor.location.channel, freeFloorId)) break;
                if (!actor.takeMove(req.codeset.error)) break;
                const auto newLocation = Location::makeFloor(roomId, actor.location.channel, freeFloorId);
                Location oldLocation;
                controller.updateCharacterLocation(actor, newLocation, oldLocation);
                result = true;
                break;
            }

            default: break;
        }
    });
    return result;
}
