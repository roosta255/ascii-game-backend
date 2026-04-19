#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <ostream>

#include "is_equal.hpp"
#include "Maybe.hpp"

typedef std::uint32_t Bitline;
constexpr static Bitline BITS_PER_LINE = sizeof(Bitline) * 8;

template<std::size_t N>
//constexpr std::uint32_t N = 128;
class Bitstick
{
public:

constexpr static Bitline BitCount = N;
constexpr static Bitline WordCount = (BitCount - 1ull) / 32ull + 1ull;

constexpr Bitstick(): _state{0}{}

constexpr Bitstick
( const Bitstick&
) = default;

constexpr Bitstick
( const std::array<Bitline, WordCount>& state
)
: _state(state)
{}

Bitstick(std::initializer_list<int> initials)
: _state{0}
{
    for(const auto& entry: initials)
        setIndexOn(entry);
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
    const Bitline bit = index % 32ull;
    const Bitline mask = 1ull << bit;

    _state[index / 32ull] |= mask;

    return *this;
}

Bitstick& setIndexOff
( const Bitline index
)
{
    const Bitline bit = index % 32ull;
    const Bitline mask = 1ull << bit;

    _state[index / 32ull] &= ~mask;

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

Bitstick operator~() const
{
    std::array<Bitline, WordCount> result;

    for(int i = 0; i < WordCount; i++)
        result[i] = ~_state[i];

    return result;
}

Maybe<bool> constexpr operator[]
( const int index
) const
{
    return index < N ? Maybe<bool>(_state[index / 32ull] & (1ull << (index % 32u))) : Maybe<bool>::empty();
}

bool inline contains(const Bitstick& rhs) const {
    for(int i = 0; i < WordCount; i++)
        if((_state[i] & rhs._state[i]) != rhs._state[i])
            return false;
    return true;
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
    return !is_equal(&_state[0], &_state[WordCount], &rhs._state[0]);
}

/*Bitstick& operator++()
{
    if(operator==(Bitstick)) return Bitstick;

    // find the first non-empty element

    // find the first bit
    const std::uint32_t bit = bit_on_uint32(_state);

    _state &= ~(1ull << bit);

    return before;
}*/

constexpr bool isAny()const
{
    const Bitstick nil;
    return !is_equal(&_state[0], &_state[WordCount], &nil._state[0]);
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
        if(i % 32 == 0)
            out << "\n";
        out << bits[i] ? "1" : "0";
    }

    return out;
}

Bitline getFirst() const { return _state[0]; }
const std::array<Bitline, WordCount>& getWords() const { return _state; }

private:

std::array<Bitline, WordCount> _state{};

};
