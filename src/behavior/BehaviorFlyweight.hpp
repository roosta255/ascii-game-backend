#pragma once

#include <string>

#include "Array.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorEventEnum.hpp"
#include "Maybe.hpp"
#include "Pointer.hpp"
#include "ProposerConfig.hpp"
#include "TransitionConfig.hpp"
#include "TriggerWrapper.hpp"

struct BehaviorFlyweight {
    struct EventTriggers {
        Maybe<TriggerWrapper> asActor;
        Maybe<TriggerWrapper> asTarget;
        Maybe<TriggerWrapper> asObserver;
    };

    const char* name = nullptr;
    Maybe<TransitionConfig> onEnter;
    Maybe<TransitionConfig> onExit;
    EventTriggers onMove;
    EventTriggers onAttack;
    EventTriggers onDamage;
    EventTriggers onLoot;
    EventTriggers onDeath;
    EventTriggers onPickpocket;
    EventTriggers onDeposit;
    Maybe<ProposerConfig> proposer;

    const EventTriggers* getTriggersForEvent(BehaviorEventEnum event) const;

    static const Array<BehaviorFlyweight, BEHAVIOR_COUNT>& getFlyweights();
    static bool indexByString(const std::string& name, BehaviorEnum& output);
};
