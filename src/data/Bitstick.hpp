#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <ostream>

#include "algorithm/any_of.hpp"
#include "algorithm/is_equal.hpp"
#include "algorithm/isnt_equal.hpp"
#include "algorithm/transform2.hpp"
#include "iterator/iterator.hpp"
#include "algorithm/byte_2_onbit.hpp"

typedef std::uint64_t Bitline;
constexpr static Bitline BITS_PER_LINE = sizeof(Bitline) * 8;

template<std::size_t N>
//constexpr std::uint64_t N = 128;
class Bitstick
{
public:

constexpr static Bitline BitCount = N;
constexpr static Bitline WordCount = (BitCount - 1ull) / 64ull + 1ull;
constexpr static Iterator Iterator = Iterator(BitCount);

constexpr Bitstick
(
)
{}

constexpr Bitstick
( const Bitstick&
) = default;

constexpr Bitstick
( const std::array<Bitline, WordCount>& state
)
: _state(state)
{}

Bitstick(std::initializer_list<Bitline> initials)
: _state{0}
{
    for(auto& state: _state)
        state = 0ull;

    for(const auto& entry: initials)
        operator|=(entry);
}

Bitstick operator|
( const Bitstick& rhsBitstick
) const
{
    std::array<Bitline, WordCount> result;

    for(int i = 0; i < WordCount; i++)
        result[i] = _state[i] | rhsBitstick._state[i];

    return result;
    //return transform2(_state, rhsBitstick._state, []
    //( const auto& lhs
    //, const auto& rhs
    //)
    //{
    //    return lhs | rhs;
    //});
}

Bitstick& setIndexOn
( const Bitline index
)
{
    const Bitline bit = index % 64ull;
    const Bitline mask = 1ull << bit;

    _state[index / 64ull] |= mask;

    return *this;
}

Bitstick& setIndexOff
( const Bitline index
)
{
    const Bitline bit = index % 64ull;
    const Bitline mask = 1ull << bit;

    _state[index / 64ull] &= ~mask;

    return *this;
}

Bitstick constexpr operator&
( const Bitstick& rhsBitstick
) const
{
    std::array<Bitline, WordCount> result;

    for(int i = 0; i < WordCount; i++)
        result[i] = _state[i] & rhsBitstick._state[i];

    return result;

    //return transform2(_state, rhsBitstick._state, []
    //( const auto& lhs
    //, const auto& rhs
    //)
    //{
    //    return lhs & rhs;
    //});
}

Bitstick& operator|=
( const Bitstick& rhsBitstick
)
{
    for(int i = 0; i < WordCount; i++)
        _state[i] = _state[i] | rhsBitstick._state[i];

    return *this;
}

Bitstick& operator&=
( const Bitstick& rhsBitstick
)
{
    for(int i = 0; i < WordCount; i++)
        _state[i] = _state[i] & rhsBitstick._state[i];

    return *this;
}

Bitstick operator-
( const Bitstick& rhsBitstick
) const
{
    std::array<Bitline, WordCount> result;

    for(int i = 0; i < WordCount; i++)
        result[i] = _state[i] & ~rhsBitstick._state[i];

    return result;

    //return transform2(_state, rhsBitstick._state, []
    //( const auto& lhs
    //, const auto& rhs
    //)
    //{
    //    return lhs & ~rhs;
    //});
}

bool constexpr operator[]
( const int index
) const
{
    return _state[index / 64ull] & (1ull << (index % 64u));
}

bool constexpr operator==
( const Bitstick& rhs
) const
{
    return is_equal(&_state[0], &_state[WordCount], &rhs._state[0]);
}

bool constexpr operator!=
( const Bitstick& rhs
) const
{
    return isnt_equal(&_state[0], &_state[WordCount], &rhs._state[0]);
}

/*Bitstick& operator++()
{
    if(operator==(Bitstick)) return Bitstick;

    // find the first non-empty element

    // find the first bit
    const std::uint64_t bit = bit_on_uint64(_state);

    _state &= ~(1ull << bit);

    return before;
}*/

explicit constexpr operator bool()const
{
    const Bitstick nil;
    return isnt_equal(&_state[0], &_state[WordCount], &nil._state[0]);
}

constexpr bool isEmpty()const
{
    const Bitstick nil;
    return is_equal(&_state[0], &_state[WordCount], &nil._state[0]);
}

Bitstick begin()const
{
    return *this;
}

Bitstick end()const
{
    const Bitstick a;
    return a;
}

friend
std::ostream& operator<<(std::ostream& out, const Bitstick<N>& bits)
{
    for(int i = 0; i < N; i++)
    {
        if(i % 8 == 0)
            out << " ";
        if(i % 64 == 0)
            out << "\n";
        out << bits[i] ? "1" : "0";
    }

    return out;
}

Bitline getFirst() const { return _state[0]; }

private:

std::array<Bitline, WordCount> _state;

};
