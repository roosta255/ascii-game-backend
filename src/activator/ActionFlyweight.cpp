#include "ActionFlyweight.hpp"
#include "ActivatorAttack.hpp"
#include "ActivatorCritterBite.hpp"
#include "ActivatorNpcAct.hpp"
#include "ActivatorDeposit.hpp"
#include "ActivatorDamage.hpp"
#include "ActivatorEndTurn.hpp"
#include "ActivatorLootChest.hpp"
#include "ActivatorUseChestLock.hpp"
#include "ActivatorMoveToDoor.hpp"
#include "ActivatorMoveToFloor.hpp"
#include "ActivatorUseCharacter.hpp"
#include "ActivatorUseDoor.hpp"
#include "ActivatorUseInventoryItem.hpp"
#include "ActivatorUseLock.hpp"
#include "Array.hpp"
#include "Bitcard.hpp"
#include "Match.hpp"
#include "iActivator.hpp"
#include "ActivatorToggler.hpp"
#include "ActivatorWrapper.hpp"
#include "WrapperConfig.hpp"
#include <unordered_map>

const Array<ActionFlyweight, ACTION_COUNT>& ActionFlyweight::getFlyweights()
{
    static auto flyweights = [](){
        Array<ActionFlyweight, ACTION_COUNT> flyweights;

        #define ACTION_DECL( name_, activation_intf_, action_type_ ) \
            static activation_intf_ GLOBAL_##name_##activation_intf_; \
            flyweights.getPointer( ACTION_##name_ ).access([&](ActionFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.activator = GLOBAL_##name_##activation_intf_; \
                flyweight.type = action_type_; \
            });

        // Declare an action whose activator is an ActivatorWrapper configured
        // inline with a WrapperConfig — same matches/conditions/costs/effects
        // vocabulary used by Rule.enum and Door.enum. The trailing arg is the
        // WrapperConfig initializer, e.g.:
        //   ACTION_WRAPPER_DECL(MY_ACTION, ACTION_TYPE_USAGE, ({
        //       .conditions = { .actor = { .required = makeTraitBits({TRAIT_ACTOR}) } },
        //       .costs = { .action = 1 },
        //       .effects = { .onSuccess = { /* ActivatorEffect... */ } }
        //   }))
        #define ACTION_WRAPPER_DECL( name_, action_type_, ... ) \
            static ActivatorWrapper GLOBAL_WRAPPER_##name_; \
            GLOBAL_WRAPPER_##name_.init( WrapperConfig __VA_ARGS__ ); \
            flyweights.getPointer( ACTION_##name_ ).access([&](ActionFlyweight& flyweight){ \
                flyweight.name = #name_; \
                flyweight.activator = GLOBAL_WRAPPER_##name_; \
                flyweight.type = action_type_; \
            });
        #include "Action.enum"
        #undef ACTION_DECL
        #undef ACTION_WRAPPER_DECL

        return flyweights;
    }();
    return flyweights;
}

bool ActionFlyweight::indexByString
(
    const std::string& name,
    ActionEnum& output
) {
    static const auto nameMap = [](){
        std::unordered_map<std::string, ActionEnum> nameMap;
        const auto& flyweights = getFlyweights();
        int i = 0;
        for(const ActionFlyweight& flyweight: flyweights) {
            nameMap[flyweight.name] = ActionEnum(i++);
        }
        return nameMap;
    }();
    if (auto search = nameMap.find(name); search != nameMap.end()) {
        output = search->second;
        return true;
    }
    return false;
}
