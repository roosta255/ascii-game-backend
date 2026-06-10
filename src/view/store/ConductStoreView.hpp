#pragma once

#include "adl_serializer.hpp"
#include "Conduct.hpp"
#include "ConductMemoryStoreView.hpp"
#include <nlohmann/json.hpp>

struct ConductStoreView
{
    int characterId = -1;
    Array<ConductMemoryStoreView, CONDUCT_COUNT> memory;

    inline ConductStoreView() = default;

    inline ConductStoreView(const Conduct& model)
    : characterId(model.characterId)
    , memory(model.memory.convert<ConductMemoryStoreView>())
    {}

    inline operator Conduct() const {
        Conduct model;
        model.characterId = this->characterId;
        model.memory = this->memory.convert<ConductMemory>();
        return model;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConductStoreView, characterId, memory)
