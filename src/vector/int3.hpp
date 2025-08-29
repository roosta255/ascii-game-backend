#pragma once

// Chrysalism
// n. the amniotic tranquility of being indoors during a thunderstorm,
// listening to waves of rain pattering against the roof like an argument upstairs,
// whose muffled words are unintelligible
// but whose crackling release of built-up tension you understand perfectly.

#include <array>

#include "vector/array.operator.hpp"

// this boyo is an int and another int
// and another one
typedef std::array<int, 3> int3;

bool constexpr operator!=
( const int3& lhs
, const int3& rhs
)
{
 return lhs[0] != rhs[0] 
 || lhs[1] != rhs[1] 
 || lhs[2] != rhs[2];
}

bool constexpr operator==
( const int3& lhs
, const int3& rhs
)
{
 return lhs[0] == rhs[0] 
 && lhs[1] == rhs[1] 
 && lhs[2] == rhs[2];
}

template<>
struct std::hash<int3>
{
    std::size_t operator()(const int3& bits)const
    {
        std::size_t seed = 0;
        for (int i = 0; i < bits.size(); ++i) {
            seed ^= static_cast<std::size_t>(bits[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct std::equal_to<int3>
{
    bool operator()(const int3& lhs, const int3& rhs)const
    {
        return lhs == rhs;
    }
};
