#pragma once

#include "adl_serializer.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorFlyweight.hpp"
#include "CodeEnum.hpp"
#include "ConductMemory.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct ConductMemoryStoreView
{
    std::string state         = "NIL";
    std::string previousState = "NIL";
    int stateChangedTurn = -1;
    int latestPriority   = -1;
    std::string lastEffectCode        = "UNSET";
    std::string lastFailureEffectCode = "UNSET";

    inline ConductMemoryStoreView() = default;

    inline ConductMemoryStoreView(const ConductMemory& model)
        : state(behavior_to_text(model.state))
        , previousState(behavior_to_text(model.previousState))
        , stateChangedTurn(model.stateChangedTurn)
        , latestPriority(model.latestPriority)
        , lastEffectCode(code_to_text(model.lastEffectCode))
        , lastFailureEffectCode(code_to_text(model.lastFailureEffectCode))
    {}

    inline operator ConductMemory() const {
        ConductMemory model;
        BehaviorFlyweight::indexByString(state, model.state);
        BehaviorFlyweight::indexByString(previousState, model.previousState);
        model.stateChangedTurn = stateChangedTurn;
        model.latestPriority   = latestPriority;
        code_from_text(lastEffectCode, model.lastEffectCode);
        code_from_text(lastFailureEffectCode, model.lastFailureEffectCode);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const ConductMemoryStoreView& v) {
    j = nlohmann::json::object();
    j["state"] = v.state;
    j["previousState"] = v.previousState;
    j["stateChangedTurn"] = v.stateChangedTurn;
    j["latestPriority"] = v.latestPriority;
    j["lastEffectCode"] = v.lastEffectCode;
    j["lastFailureEffectCode"] = v.lastFailureEffectCode;
}

inline void from_json(const nlohmann::json& j, ConductMemoryStoreView& v) {
    if (j.contains("state"))                  j.at("state").get_to(v.state);
    if (j.contains("previousState"))           j.at("previousState").get_to(v.previousState);
    if (j.contains("stateChangedTurn"))        j.at("stateChangedTurn").get_to(v.stateChangedTurn);
    if (j.contains("latestPriority"))          j.at("latestPriority").get_to(v.latestPriority);
    if (j.contains("lastEffectCode"))          j.at("lastEffectCode").get_to(v.lastEffectCode);
    if (j.contains("lastFailureEffectCode"))   j.at("lastFailureEffectCode").get_to(v.lastFailureEffectCode);
}
