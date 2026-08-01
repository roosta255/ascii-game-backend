#include "ActivationContext.hpp"
#include "LocationEnum.hpp"
#include "RoleFlyweight.hpp"

namespace {

TraitBits relationshipTraitBits() {
    return makeTraitBits({ TRAIT_PRIMARY, TRAIT_SECONDARY, TRAIT_TERTIARY });
}

Maybe<TraitEnum> relationTrait(CharacterRelation relation) {
    switch (relation) {
        case CharacterRelation::Primary:   return Maybe<TraitEnum>(TRAIT_PRIMARY);
        case CharacterRelation::Secondary: return Maybe<TraitEnum>(TRAIT_SECONDARY);
        case CharacterRelation::Tertiary:  return Maybe<TraitEnum>(TRAIT_TERTIARY);
        default:                           return Maybe<TraitEnum>();
    }
}

// Floor-anchored searches scan the anchor's room; chest-anchored searches are
// restricted to characters contained in that same chest. New locations can be
// added here without changing any activator.
Pointer<Character> findPairedCharacter(Dungeon& dungeon, const Character& source, RoleEnum pairedRole) {
    if (source.location.type == LOCATION_CHEST) {
        for (Character& ch : dungeon.characters) {
            if (ch.role == pairedRole &&
                ch.location.type == LOCATION_CHEST &&
                ch.location.data == source.location.data)
                return ch;
        }
        return {};
    }

    for (Character& ch : dungeon.characters) {
        if (ch.role == pairedRole && ch.location.roomId == source.location.roomId)
            return ch;
    }
    return {};
}

} // namespace

Pointer<Character> ActivationContext::resolveCharacter(CharacterAnchor anchor, CharacterRelation relation) const {
    Pointer<Character> anchorCharacter;
    switch (anchor) {
        case CharacterAnchor::Actor:
            anchorCharacter = character;
            break;
        case CharacterAnchor::Target:
            anchorCharacter = targetCharacter();
            break;
    }

    if (anchorCharacter.isEmpty() || relation == CharacterRelation::Self)
        return anchorCharacter;

    Maybe<TraitEnum> wantedTrait = relationTrait(relation);
    if (wantedTrait.isEmpty())
        return {};

    Pointer<Character> result;
    anchorCharacter.access([&](Character& source) {
        TraitBits currentTraits;
        RoleFlyweight::getFlyweights().accessConst((int)source.role, [&](const RoleFlyweight& fw) {
            currentTraits = fw.traitsSourced;
        });

        TraitBits desired = (currentTraits - relationshipTraitBits());
        wantedTrait.access([&](TraitEnum trait) {
            desired = desired | makeTraitBits({ trait });
        });

        RoleEnum pairedRole;
        if (!RoleFlyweight::findByTraits(desired, pairedRole))
            return;

        request.access([&](RequestContext& req) {
            result = findPairedCharacter(req.match.dungeon, source, pairedRole);
        });
    });
    return result;
}
