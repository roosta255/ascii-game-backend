#pragma once

// Vemödalen
// n. 
// the frustration of photographing something amazing
// when thousands of identical photos already exist—the same sunset,
// the same waterfall,
// the same curve of a hip,
// the same closeup of an eye—
// which can turn a unique subject
// into something hollow and pulpy and cheap,
// like a mass-produced piece of furniture
// you happen to have assembled yourself.

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "array.operator.hpp"


// this boyo is an int and another int
// and another one
typedef std::array<int, 4> int4;

bool constexpr operator!=
( const int4& lhs
, const int4& rhs
)
{
 return lhs[0] != rhs[0]
 || lhs[1] != rhs[1]
 || lhs[2] != rhs[2]
 || lhs[3] != rhs[3];
}

bool constexpr operator==
( const int4& lhs
, const int4& rhs
)
{
 return lhs[0] == rhs[0]
 && lhs[1] == rhs[1]
 && lhs[2] == rhs[2]
 && lhs[3] == rhs[3];
}

template<>
struct std::hash<int4>
{
    std::size_t operator()(const int4& bits)const
    {
        std::size_t seed = 0;
        for (int i = 0; i < bits.size(); ++i) {
            seed ^= static_cast<std::size_t>(bits[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct std::equal_to<int4>
{
    bool operator()(const int4& lhs, const int4& rhs)const
    {
        return lhs == rhs;
    }
};
