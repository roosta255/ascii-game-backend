#pragma once

#include <string>

#include "Array.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorEventEnum.hpp"
#include "iProposer.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "TriggerWrapper.hpp"

struct BehaviorFlyweight {
    const char* name = nullptr;
    Maybe<TriggerWrapper> onMove;
    Maybe<TriggerWrapper> onAttack;
    Maybe<TriggerWrapper> onDamage;
    Maybe<TriggerWrapper> onLoot;
    Maybe<TriggerWrapper> onDeath;
    Maybe<TriggerWrapper> onPickpocket;
    Maybe<TriggerWrapper> onDeposit;
    Pointer<const iProposer> proposer;

    Maybe<TriggerWrapper> getActivatorForEvent(BehaviorEventEnum event) const;

    static const Array<BehaviorFlyweight, BEHAVIOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, BehaviorEnum& output);
};
