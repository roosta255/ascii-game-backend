#include "BehaviorFlyweight.hpp"
#include "Array.hpp"
#include "iActivator.hpp"
#include <unordered_map>

Pointer<const iActivator> BehaviorFlyweight::getActivatorForEvent(BehaviorEventEnum event) const {
    switch (event) {
        case BEHAVIOR_EVENT_MOVE:   return onMove;
        case BEHAVIOR_EVENT_ATTACK: return onAttack;
        case BEHAVIOR_EVENT_DAMAGE: return onDamage;
        case BEHAVIOR_EVENT_LOOT:   return onLoot;
        case BEHAVIOR_EVENT_DEATH:  return onDeath;
        default:                    return Pointer<const iActivator>::empty();
    }
}

const Array<BehaviorFlyweight, BEHAVIOR_COUNT>& BehaviorFlyweight::getFlyweights() {
    static auto flyweights = [](){
        Array<BehaviorFlyweight, BEHAVIOR_COUNT> flyweights;
        BehaviorEnum lastBehavior = BEHAVIOR_NIL;

        #define BEHAVIOR_DECL(name_) \
            lastBehavior = BEHAVIOR_##name_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.name = #name_; \
            });
        #define BEHAVIOR_ON_MOVE_DECL(activator_) \
            static activator_ GLOBAL_##activator_##_onMove_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onMove = GLOBAL_##activator_##_onMove_; \
            });
        #define BEHAVIOR_ON_ATTACK_DECL(activator_) \
            static activator_ GLOBAL_##activator_##_onAttack_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onAttack = GLOBAL_##activator_##_onAttack_; \
            });
        #define BEHAVIOR_ON_DAMAGE_DECL(activator_) \
            static activator_ GLOBAL_##activator_##_onDamage_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDamage = GLOBAL_##activator_##_onDamage_; \
            });
        #define BEHAVIOR_ON_LOOT_DECL(activator_) \
            static activator_ GLOBAL_##activator_##_onLoot_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onLoot = GLOBAL_##activator_##_onLoot_; \
            });
        #define BEHAVIOR_ON_DEATH_DECL(activator_) \
            static activator_ GLOBAL_##activator_##_onDeath_; \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeath = GLOBAL_##activator_##_onDeath_; \
            });

        #include "Behavior.enum"

        #undef BEHAVIOR_DECL
        #undef BEHAVIOR_ON_MOVE_DECL
        #undef BEHAVIOR_ON_ATTACK_DECL
        #undef BEHAVIOR_ON_DAMAGE_DECL
        #undef BEHAVIOR_ON_LOOT_DECL
        #undef BEHAVIOR_ON_DEATH_DECL

        return flyweights;
    }();
    return flyweights;
}

bool BehaviorFlyweight::indexByString(const std::string& name, BehaviorEnum& output) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, BehaviorEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for (const BehaviorFlyweight& fw : flyweights) {
            nameMap[fw.name] = BehaviorEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
