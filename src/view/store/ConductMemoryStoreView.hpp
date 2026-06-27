#pragma once

#include "adl_serializer.hpp"
#include "BehaviorEnum.hpp"
#include "BehaviorFlyweight.hpp"
#include "ConductMemory.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct ConductMemoryStoreView
{
    std::string state = "NIL";

    inline ConductMemoryStoreView() = default;

    inline ConductMemoryStoreView(const ConductMemory& model)
        : state(behavior_to_text(model.state))
    {}

    inline operator ConductMemory() const {
        ConductMemory model;
        BehaviorFlyweight::indexByString(state, model.state);
        return model;
    }
};

inline void to_json(nlohmann::json& j, const ConductMemoryStoreView& v) {
    j = nlohmann::json::object();
    j["state"] = v.state;
}

inline void from_json(const nlohmann::json& j, ConductMemoryStoreView& v) {
    if (j.contains("state")) j.at("state").get_to(v.state);
}
