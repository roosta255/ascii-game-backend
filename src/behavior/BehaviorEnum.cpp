#include "BehaviorEnum.hpp"

const char* behavior_to_text(int index) {
    static const char* names[] = {
        #define BEHAVIOR_DECL(name_) #name_,
        #define BEHAVIOR_ON_MOVE_DECL(config_)
        #define BEHAVIOR_ON_ATTACK_DECL(config_)
        #define BEHAVIOR_ON_DAMAGE_DECL(config_)
        #define BEHAVIOR_ON_LOOT_DECL(config_)
        #define BEHAVIOR_ON_DEATH_DECL(config_)
        #define BEHAVIOR_ON_PICKPOCKET_DECL(config_)
        #define BEHAVIOR_ON_DEPOSIT_DECL(config_)
        #define BEHAVIOR_ON_ACT_DECL(config_)
        #define BEHAVIOR_PROPOSER_DECL(type_)
        #include "Behavior.enum"
        #undef BEHAVIOR_DECL
        #undef BEHAVIOR_ON_MOVE_DECL
        #undef BEHAVIOR_ON_ATTACK_DECL
        #undef BEHAVIOR_ON_DAMAGE_DECL
        #undef BEHAVIOR_ON_LOOT_DECL
        #undef BEHAVIOR_ON_DEATH_DECL
        #undef BEHAVIOR_ON_PICKPOCKET_DECL
        #undef BEHAVIOR_ON_DEPOSIT_DECL
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
