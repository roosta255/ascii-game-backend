#include "BehaviorFlyweight.hpp"
#include "Array.hpp"
#include "ProposalEffect.hpp"
#include "ProposerConfig.hpp"
#include "TriggerConfig.hpp"
#include "TriggerWrapper.hpp"

#include <unordered_map>

// ─── Flyweight table ─────────────────────────────────────────────────────────

const BehaviorFlyweight::EventTriggers* BehaviorFlyweight::getTriggersForEvent(BehaviorEventEnum event) const {
    switch (event) {
        case BEHAVIOR_EVENT_MOVE:        return &onMove;
        case BEHAVIOR_EVENT_ATTACK:      return &onAttack;
        case BEHAVIOR_EVENT_DAMAGE:      return &onDamage;
        case BEHAVIOR_EVENT_LOOT:        return &onLoot;
        case BEHAVIOR_EVENT_DEATH:       return &onDeath;
        case BEHAVIOR_EVENT_PICKPOCKET:  return &onPickpocket;
        case BEHAVIOR_EVENT_DEPOSIT:     return &onDeposit;
        default:                         return nullptr;
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

        #define BEHAVIOR_ON_MOVE_AS_ACTOR_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onMove.asActor = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_MOVE_AS_TARGET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onMove.asTarget = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_MOVE_AS_OBSERVER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onMove.asObserver = TriggerWrapper{TriggerConfig config_}; \
            });

        #define BEHAVIOR_ON_ATTACK_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_AS_OBSERVER_DECL(config_)

        #define BEHAVIOR_ON_DAMAGE_AS_ACTOR_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDamage.asActor = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DAMAGE_AS_TARGET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDamage.asTarget = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DAMAGE_AS_OBSERVER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDamage.asObserver = TriggerWrapper{TriggerConfig config_}; \
            });

        #define BEHAVIOR_ON_LOOT_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_LOOT_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_LOOT_AS_OBSERVER_DECL(config_)

        #define BEHAVIOR_ON_DEATH_AS_ACTOR_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeath.asActor = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DEATH_AS_TARGET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeath.asTarget = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DEATH_AS_OBSERVER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeath.asObserver = TriggerWrapper{TriggerConfig config_}; \
            });

        #define BEHAVIOR_ON_PICKPOCKET_AS_ACTOR_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onPickpocket.asActor = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_PICKPOCKET_AS_TARGET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onPickpocket.asTarget = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_PICKPOCKET_AS_OBSERVER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onPickpocket.asObserver = TriggerWrapper{TriggerConfig config_}; \
            });

        #define BEHAVIOR_ON_DEPOSIT_AS_ACTOR_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeposit.asActor = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DEPOSIT_AS_TARGET_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeposit.asTarget = TriggerWrapper{TriggerConfig config_}; \
            });
        #define BEHAVIOR_ON_DEPOSIT_AS_OBSERVER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.onDeposit.asObserver = TriggerWrapper{TriggerConfig config_}; \
            });

        #define BEHAVIOR_ON_ACT_DECL(config_)
        #define BEHAVIOR_PROPOSER_DECL(config_) \
            flyweights.getPointer(lastBehavior).access([&](BehaviorFlyweight& fw){ \
                fw.proposer = ProposerConfig config_; \
            });

        #include "Behavior.enum"

        #undef BEHAVIOR_DECL
        #undef BEHAVIOR_ON_MOVE_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_MOVE_AS_TARGET_DECL
        #undef BEHAVIOR_ON_MOVE_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_ATTACK_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_ATTACK_AS_TARGET_DECL
        #undef BEHAVIOR_ON_ATTACK_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_DAMAGE_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_DAMAGE_AS_TARGET_DECL
        #undef BEHAVIOR_ON_DAMAGE_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_LOOT_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_LOOT_AS_TARGET_DECL
        #undef BEHAVIOR_ON_LOOT_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_DEATH_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_DEATH_AS_TARGET_DECL
        #undef BEHAVIOR_ON_DEATH_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_PICKPOCKET_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_PICKPOCKET_AS_TARGET_DECL
        #undef BEHAVIOR_ON_PICKPOCKET_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_DEPOSIT_AS_ACTOR_DECL
        #undef BEHAVIOR_ON_DEPOSIT_AS_TARGET_DECL
        #undef BEHAVIOR_ON_DEPOSIT_AS_OBSERVER_DECL
        #undef BEHAVIOR_ON_ACT_DECL
        #undef BEHAVIOR_PROPOSER_DECL

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
