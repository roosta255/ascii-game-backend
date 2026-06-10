#pragma once

#include "adl_serializer.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorFlyweight.hpp"
#include "ConductMemory.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct ConductMemoryStoreView
{
    int targetCharacterId = -1;
    int targetObjectId    = -1;
    int targetRoomId      = -1;
    std::string state     = "BEHAVIOR_NIL";

    inline ConductMemoryStoreView() = default;

    inline ConductMemoryStoreView(const ConductMemory& model)
    : targetCharacterId(model.targetCharacterId)
    , targetObjectId(model.targetObjectId)
    , targetRoomId(model.targetRoomId)
    , state(behavior_to_text(model.state))
    {}

    inline operator ConductMemory() const {
        ConductMemory model{
            .targetCharacterId = this->targetCharacterId,
            .targetObjectId    = this->targetObjectId,
            .targetRoomId      = this->targetRoomId,
            .state             = BEHAVIOR_NIL,
        };
        BehaviorFlyweight::indexByString(this->state, model.state);
        return model;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConductMemoryStoreView, targetCharacterId, targetObjectId, targetRoomId, state)
