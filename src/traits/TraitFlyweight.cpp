#include "Array.hpp"
#include "TraitFlyweight.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include <unordered_map>

const Array<TraitFlyweight, TRAIT_COUNT>& TraitFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<TraitFlyweight, TRAIT_COUNT> flyweights;
        TraitEnum lastTrait = TRAIT_NIL; // Tracker for the current "Parent" trait

        #define TRAIT_DECL( enum_, type_ ) \
            lastTrait = TRAIT_##enum_; \
            flyweights.getPointer( lastTrait ).access([&](TraitFlyweight& f){ \
                f.name = #enum_; \
                f.type = type_; \
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

        return flyweights;
    }();
    return flyweights;
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
