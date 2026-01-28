#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "array.operator.hpp"

// this boyo is an int and another int
typedef std::array<int, 2> int2;

bool constexpr operator!=
( const int2& lhs
, const int2& rhs
)
{
 return lhs[0] != rhs[0]
 || lhs[1] != rhs[1];
}

bool constexpr operator==
( const int2& lhs
, const int2& rhs
)
{
 return lhs[0] == rhs[0]
 && lhs[1] == rhs[1];
}

template<>
struct std::hash<int2>
{
    std::size_t operator()(const int2& bits)const
    {
        std::size_t seed = 0;
        for (int i = 0; i < bits.size(); ++i) {
            seed ^= static_cast<std::size_t>(bits[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct std::equal_to<int2>
{
    bool operator()(const int2& lhs, const int2& rhs)const
    {
        return lhs == rhs;
    }
};
