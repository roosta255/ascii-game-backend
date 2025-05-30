#pragma once

#include <cstdint>

#include "math/bit_roll.hpp"

constexpr std::uint64_t twist_3
( const std::uint64_t& state
)
{
    return bit_roll(((state >> 18u) ^ state) >> 27u, state >> 59u);
}

constexpr std::uint64_t twist_2
( const std::uint64_t& left
, const std::uint64_t& right
)
{
    return twist_3(left * 6364136223846793005ULL + (right|1));
}

constexpr std::uint64_t twist
( const std::uint64_t& lhs
, const std::uint64_t& rhs
)
{
    return twist_2(bit_roll(lhs, rhs), bit_roll(rhs, lhs));
}
/*
std::uint64_t twist
( const std::uint64_t& lhs
, const std::uint64_t& rhs
)
{
    const auto left = bit_roll(lhs, rhs);
    const auto right = bit_roll(rhs, lhs);

    const auto state = left * 6364136223846793005ULL + (right|1);

    // Calculate output function (XSH RR), uses old state for max ILP
    const auto xorshifted = ((state >> 18u) ^ state) >> 27u;

    const auto rot = state >> 59u;

    const auto twisted = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    return bit_roll(twisted, rot);
}
*/
template<typename... Tail>
constexpr std::uint64_t twist
( const std::uint64_t& head
, const std::uint64_t& headpp
, const Tail&... tail)
{
    return twist(twist(head, headpp), tail...);
}
