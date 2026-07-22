#include "DungeonRoleTransformer.hpp"
#include "Character.hpp"
#include "Inventory.hpp"
#include "Match.hpp"
#include "RequestContext.hpp"
#include "RoleFlyweight.hpp"

bool DungeonRoleTransformer::activate(ActivationContext& activation) const {
    activation.request.access([&](RequestContext& req) {
        auto applyToCharacter = [&](Character& ch) {
            if (assignment.directRole != ROLE_COUNT) {
                ch.role = assignment.directRole;
                return;
            }
            RoleFlyweight::getFlyweights().accessConst((int)ch.role, [&](const RoleFlyweight& fw) {
                if ((assignment.match - fw.traitsSourced).isAny()) return;
                const TraitBits newTraits = (fw.traitsSourced | assignment.set) - assignment.clear;
                if (newTraits == fw.traitsSourced) return;
                RoleEnum newRole;
                if (RoleFlyweight::findByTraits(newTraits, newRole))
                    ch.role = newRole;
            });
        };

        if (relation.isPresent()) {
            relation.accessConst([&](CharacterRelation rel) {
                activation.resolveCharacter(anchor, rel).access(applyToCharacter);
            });
            return;
        }

        switch (scope) {
            case CharacterScope::Object:
                applyToCharacter(activation.character);
                break;

            case CharacterScope::Target:
                activation.targetCharacter().access([&](Character& target) { applyToCharacter(target); });
                break;

            case CharacterScope::Tool:
                activation.sourceItem.access([&](Item& item) {
                    const Item* itemPtr = &item;
                    for (Character& ch : req.match.dungeon.characters) {
                        const auto inv = ch.getInventory(req.match.dungeon);
                        if (!inv.isValid()) continue;
                        if (itemPtr >= inv.items && itemPtr < inv.items + inv.size) {
                            applyToCharacter(ch);
                            break;
                        }
                    }
                });
                break;

            case CharacterScope::Room: {
                const int roomId = activation.character.location.roomId;
                for (Character& ch : req.match.dungeon.characters) {
                    if (ch.location.roomId == roomId)
                        applyToCharacter(ch);
                }
                break;
            }

            case CharacterScope::Dungeon:
                for (Character& ch : req.match.dungeon.characters)
                    applyToCharacter(ch);
                break;
        }
    });
    return true;
}
