#include "Array.hpp"
#include "TraitFlyweight.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include <unordered_map>

Bitstick<TRAIT_TYPE_COUNT> makeTraitTypes(std::initializer_list<TraitType> setList) {
    Bitstick<TRAIT_TYPE_COUNT> bits;
    for (auto t : setList)
    {
        bits.setIndexOn((size_t)t);
    }
    return bits;
}

const Array<TraitFlyweight, TRAIT_COUNT>& TraitFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<TraitFlyweight, TRAIT_COUNT> flyweights;
        TraitEnum lastTrait = TRAIT_NIL; // Tracker for the current "Parent" trait

        #define TRAIT_DECL( enum_, types_ ) \
            lastTrait = TRAIT_##enum_; \
            flyweights.getPointer( lastTrait ).access([&](TraitFlyweight& f){ \
                f.name = #enum_; \
                f.index = TRAIT_##enum_; \
                f.types = makeTraitTypes types_; \
            });

        #define TRAIT_MODIFIER_DECL( is_global_, modifier_ ) \
            flyweights.getPointer( lastTrait ).access([&](TraitFlyweight& f){ \
                auto modifier = TraitModifier modifier_; \
                modifier.isGlobal = is_global_; \
                f.modifiers.push_back(modifier); \
            });

        #include "Trait.enum"
        #undef TRAIT_DECL
        #undef TRAIT_MODIFIER_DECL

        // upstream/downstream
        for (auto& flyweight: flyweights) {
            for (auto& modifier: flyweight.modifiers) {
                flyweight.downstreamClearers = flyweight.downstreamClearers | modifier.clear;
                flyweight.downstreamSetters = flyweight.downstreamSetters | modifier.set;
                for (int t = 0; t < TRAIT_COUNT; t++) {
                    flyweights.access(t, [&](auto& targetFlyweight){
                        if (modifier.clear[t].orElse(false)) {
                            targetFlyweight.upstreamClearers.setIndexOn((Bitline)flyweight.index);
                        }
                        if (modifier.set[t].orElse(false)) {
                            targetFlyweight.upstreamSetters.setIndexOn((Bitline)flyweight.index);
                        }
                    });
                }
            }
        }

        return flyweights;
    }();
    return flyweights;
}

TraitBits TraitFlyweight::getDownstream(const TraitBits& computed) const {
    return (computed[index].orElse(false) ? downstreamSetters : downstreamClearers) & computed;
}

TraitBits TraitFlyweight::getUpstream(const TraitBits& computed) const {
    return (computed[index].orElse(false) ? upstreamSetters : upstreamClearers) & computed;
}

bool TraitFlyweight::indexByString
(
    const std::string& name,
    TraitEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, TraitEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const TraitFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = TraitEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
