#pragma once

#include "adl_serializer.hpp"
#include "Cell.hpp"
#include <string>
#include <nlohmann/json.hpp>

struct CellApiView
{
    int offset = 0;
    int x = 0, y = 0;

    inline CellApiView() = default;
 
    inline CellApiView(const Cell& model, const MatchApiParameters& params, const int x, const int y)
    : offset(model.offset), x(x), y(y)
    {}
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CellApiView, offset, x, y)
