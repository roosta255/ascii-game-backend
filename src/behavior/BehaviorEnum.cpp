#include "BehaviorEnum.hpp"

const char* behavior_to_text(int index) {
    static const char* names[] = {
        #define BEHAVIOR_DECL(name_) #name_,
        #define BEHAVIOR_ON_MOVE_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_MOVE_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_MOVE_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_DAMAGE_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_DAMAGE_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_DAMAGE_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_LOOT_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_LOOT_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_LOOT_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_DEATH_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_DEATH_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_DEATH_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_PICKPOCKET_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_PICKPOCKET_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_PICKPOCKET_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_DEPOSIT_AS_ACTOR_DECL(config_)
        #define BEHAVIOR_ON_DEPOSIT_AS_TARGET_DECL(config_)
        #define BEHAVIOR_ON_DEPOSIT_AS_OBSERVER_DECL(config_)
        #define BEHAVIOR_ON_ENTER_DECL(effects_)
        #define BEHAVIOR_ON_EXIT_DECL(effects_)
        #define BEHAVIOR_ON_ACT_DECL(config_)
        #define BEHAVIOR_PROPOSER_DECL(type_)
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
        #undef BEHAVIOR_ON_ENTER_DECL
        #undef BEHAVIOR_ON_EXIT_DECL
        #undef BEHAVIOR_ON_ACT_DECL
        #undef BEHAVIOR_PROPOSER_DECL
    };
    if (index < 0) {
        return "BEHAVIOR_INDEX_NEGATIVE";
    }
    if (index >= BEHAVIOR_COUNT) {
        return "BEHAVIOR_INDEX_OUT_OF_BOUNDS";
    }
    return names[index];
}
