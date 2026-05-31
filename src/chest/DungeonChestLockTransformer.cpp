#include "DungeonChestLockTransformer.hpp"
#include "Character.hpp"
#include "Chest.hpp"
#include "LockFlyweight.hpp"
#include "Match.hpp"
#include "MatchController.hpp"

bool DungeonChestLockTransformer::activate(ActivationContext& activation) const {
    activation.request.access([&](RequestContext& req) {
        auto applyToChest = [&](Chest& chest) {
            if (assignment.directLock != LOCK_NIL) {
                chest.lock = assignment.directLock;
                return;
            }
            LockFlyweight::getFlyweights().accessConst((int)chest.lock, [&](const LockFlyweight& fw) {
                if ((assignment.match - fw.lockAttributes).isAny()) return;
                const TraitBits newTraits = (fw.lockAttributes | assignment.set) - assignment.clear;
                if (newTraits == fw.lockAttributes) return;
                LockEnum newLock;
                if (LockFlyweight::findByTraits(newTraits, newLock))
                    chest.lock = newLock;
            });
        };

        switch (scope) {
            case ChestScope::Subject:
                activation.sourceChest().access([&](Chest& chest) { applyToChest(chest); });
                break;

            case ChestScope::Target:
                activation.targetChest().access([&](Chest& chest) { applyToChest(chest); });
                break;

            case ChestScope::Room: {
                const int roomId = activation.room.roomId;
                for (Chest& chest : req.match.dungeon.chests) {
                    if (chest.containerCharacterId == -1) continue;
                    for (Character& character : req.match.dungeon.characters) {
                        if (character.characterId == chest.containerCharacterId) {
                            if (character.location.roomId == roomId)
                                applyToChest(chest);
                            break;
                        }
                    }
                }
                break;
            }

            case ChestScope::Dungeon:
                for (Chest& chest : req.match.dungeon.chests) {
                    if (chest.containerCharacterId != -1)
                        applyToChest(chest);
                }
                break;
        }
    });
    return true;
}
