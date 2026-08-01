#pragma once

#include "adl_serializer.hpp"
#include "Conduct.hpp"
#include "ConductMemoryStoreView.hpp"
#include "ConductMemoryVariableEnum.hpp"
#include <nlohmann/json.hpp>

struct ConductStoreView
{
    int characterId = -1;
    int vars[CONDUCT_MEMORY_NPC_VARIABLE_COUNT];
    Array<ConductMemoryStoreView, CONDUCT_COUNT> memory;

    inline ConductStoreView() noexcept : characterId(-1) {
        for (int& v : vars) v = -1;
    }

    inline ConductStoreView(const Conduct& model)
    : characterId(model.characterId)
    , memory(model.memory.convert<ConductMemoryStoreView>())
    {
        for (int i = 0; i < CONDUCT_MEMORY_NPC_VARIABLE_COUNT; i++) vars[i] = model.vars[i];
    }

    inline operator Conduct() const {
        Conduct model;
        model.characterId = this->characterId;
        model.memory = this->memory.convert<ConductMemory>();
        for (int i = 0; i < CONDUCT_MEMORY_NPC_VARIABLE_COUNT; i++) model.vars[i] = vars[i];
        return model;
    }
};

inline void to_json(nlohmann::json& j, const ConductStoreView& v) {
    j = nlohmann::json::object();
    j["characterId"] = v.characterId;
    j["memory"] = v.memory;
    nlohmann::json varsObj = nlohmann::json::object();
    for (int i = 0; i < CONDUCT_MEMORY_NPC_VARIABLE_COUNT; i++) {
        const char* key = conduct_memory_variable_to_text(
            ConductMemoryVariableEnum(CONDUCT_MEMORY_NPC_VARIABLE_BASE + i));
        varsObj[key] = v.vars[i];
    }
    j["vars"] = varsObj;
}

inline void from_json(const nlohmann::json& j, ConductStoreView& v) {
    if (j.contains("characterId")) j.at("characterId").get_to(v.characterId);
    if (j.contains("memory"))      j.at("memory").get_to(v.memory);
    if (j.contains("vars")) {
        const auto& varsObj = j.at("vars");
        for (int i = 0; i < CONDUCT_MEMORY_NPC_VARIABLE_COUNT; i++) {
            const char* key = conduct_memory_variable_to_text(
                ConductMemoryVariableEnum(CONDUCT_MEMORY_NPC_VARIABLE_BASE + i));
            if (varsObj.contains(key)) v.vars[i] = varsObj.at(key).get<int>();
        }
    }
}
