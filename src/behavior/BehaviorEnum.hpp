#pragma once

#include <ostream>
#include <string>

#define BEHAVIOR_DECL(name_) BEHAVIOR_##name_,
#define BEHAVIOR_ON_MOVE_DECL(config_)
#define BEHAVIOR_ON_ATTACK_DECL(config_)
#define BEHAVIOR_ON_DAMAGE_DECL(config_)
#define BEHAVIOR_ON_LOOT_DECL(config_)
#define BEHAVIOR_ON_DEATH_DECL(config_)
#define BEHAVIOR_ON_PICKPOCKET_DECL(config_)
#define BEHAVIOR_ON_DEPOSIT_DECL(config_)
#define BEHAVIOR_ON_ACT_DECL(config_)
#define BEHAVIOR_PROPOSER_DECL(type_)
enum BehaviorEnum : int {
#include "Behavior.enum"
    BEHAVIOR_COUNT
};
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

const char* behavior_to_text(int);

inline std::ostream& operator<<(std::ostream& os, BehaviorEnum b) {
    return os << behavior_to_text(b);
}
