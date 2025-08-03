#pragma once

#include "adl_serializer.hpp"
#include "Cell.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct CellStoreView
{
    int offset = 0;

    inline CellStoreView() = default;
 
    inline CellStoreView(const Cell& model)
    : offset(model.offset)
    {}
  
    inline operator Cell() const {
        return Cell{
            .offset = this->offset
        };
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CellStoreView, offset)
