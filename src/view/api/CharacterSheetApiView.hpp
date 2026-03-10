#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "Character.hpp"
#include "RoleFlyweight.hpp"
#include "TraitBits.hpp"
#include "TraitEnum.hpp"
#include "TraitFlyweight.hpp"
#include "TraitModifier.hpp"
#include "TraitType.hpp"

// Full structured character sheet contract. Backend computes all trait deltas
// and capability analysis; frontend converts to text.
struct CharacterSheetApiView
{
    struct StreamedTraitView {
        bool isPresent;
        std::string trait;
        std::vector<std::string> upstream;
        std::vector<std::string> downstream;
    };

    std::vector<StreamedTraitView> afflictions, attributes, capabilities, characters, debuffs, states, roles;

    CharacterSheetApiView() = default;

    CharacterSheetApiView(const Character& character, const TraitModifier::TraitComputation& computed)
    {
        const TraitBits& final = computed.final;

        // streamed traits
        const auto traitFlyweights = TraitFlyweight::getFlyweights();
        const auto makeTraitVector = [&](const TraitBits& traits, const int capacity){
            std::vector<std::string> temp;
            for (int t = 0; t < TRAIT_COUNT; t++) {
                if (traits[t].orElse(false)) {
                    traitFlyweights.accessConst(t, [&](const TraitFlyweight& flyweight){
                        temp.push_back(flyweight.name);
                    });
                }
                if (temp.size() >= capacity) {
                    return temp;
                }
            }
            return temp;
        };
        const auto pushStreamedTraitView = [&](const auto& flyweight, std::vector<StreamedTraitView>& list){
            const bool isPresent = final[flyweight.index].orElse(false);
            if (isPresent || computed.cleared[flyweight.index].orElse(false)) {
                list.push_back(StreamedTraitView{
                    .isPresent = isPresent,
                    .trait = flyweight.name,
                    .upstream = makeTraitVector(flyweight.getUpstream(final), 2),
                    .downstream = makeTraitVector(flyweight.getUpstream(final), 2)
                });
            }
        };
        for (const auto& flyweight: traitFlyweights) {
            if (flyweight.types[TRAIT_TYPE_UNSHEETED].orElse(false)) {
                continue;
            }
            if (flyweight.types[TRAIT_TYPE_AFFLICTION].orElse(false)) {
                pushStreamedTraitView(flyweight, afflictions);
            }
            if (flyweight.types[TRAIT_TYPE_ATTRIBUTE].orElse(false)) {
                pushStreamedTraitView(flyweight, attributes);
            }
            if (flyweight.types[TRAIT_TYPE_CAPABILITY].orElse(false)) {
                pushStreamedTraitView(flyweight, capabilities);
            }
            if (flyweight.types[TRAIT_TYPE_CHARACTER].orElse(false)) {
                pushStreamedTraitView(flyweight, characters);
            }
            if (flyweight.types[TRAIT_TYPE_DEBUFF].orElse(false)) {
                pushStreamedTraitView(flyweight, debuffs);
            }
            if (flyweight.types[TRAIT_TYPE_STATE].orElse(false)) {
                pushStreamedTraitView(flyweight, states);
            }
        }

        // role traits: present traits, cleared role traits
        RoleFlyweight::getFlyweights().accessConst(character.role, [&](const RoleFlyweight& roleFlyweight){
            const auto traitsRemaining = roleFlyweight.traitsSourced & final;
            const auto traitsLost = (final - traitsRemaining);
            for (const auto traitFlyweight: traitFlyweights) {
                if (traitsRemaining[traitFlyweight.index].orElse(false)) {
                    roles.push_back(StreamedTraitView{
                        .isPresent = true,
                        .trait = traitFlyweight.name,
                        .downstream = makeTraitVector(traitFlyweight.getUpstream(final), 2)
                    });
                }
                if (traitsLost[traitFlyweight.index].orElse(false)) {
                    roles.push_back(StreamedTraitView{
                        .isPresent = false,
                        .trait = traitFlyweight.name,
                        .upstream = makeTraitVector(traitFlyweight.getUpstream(final), 2),
                        .downstream = makeTraitVector(traitFlyweight.getUpstream(final), 2)
                    });
                }
            }
        });
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CharacterSheetApiView::StreamedTraitView, isPresent, trait, upstream, downstream)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CharacterSheetApiView, afflictions, attributes, capabilities, characters, debuffs, states, roles)
