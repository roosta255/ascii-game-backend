#pragma once

#include <cstdint>
#include <initializer_list>
#include <ostream>

#include "algorithm/byte_2_onbit.hpp"

class Bitstick32
{
Bitstick32(std::uint32_t initials)
: _state(initials)
{
}

public:

constexpr static Bitstick32 buildIndex(std::uint32_t init)
{
    return Bitstick32(1 << init);
}

constexpr static Bitstick32 buildMask(std::uint32_t init)
{
    return Bitstick32(init);
}

constexpr Bitstick32() = default;
constexpr Bitstick32(const Bitstick32&) = default;

constexpr Bitstick32 operator| (const Bitstick32& rhsBitstick) const
{
    return Bitstick32(_state | rhsBitstick);
}

Bitstick32& operator|=(const Bitstick32& rhsBitstick)
{
    _state |= rhsBitstick;

    return *this;
}

Bitstick32 constexpr operator&(const Bitstick32& rhsBitstick) const
{
    std::uint32_t result;

    for(int i = 0; i < WordCount; i++)
        result[i] = _state[i] & rhsBitstick._state[i];

    return result;
}

Bitstick32& operator|=(const Bitstick32& rhsBitstick)
{
    for(int i = 0; i < WordCount; i++)
        _state[i] = _state[i] | rhsBitstick._state[i];

    return *this;
}

Bitstick32& operator&=(const Bitstick32& rhsBitstick)
{
    for(int i = 0; i < WordCount; i++)
        _state[i] = _state[i] & rhsBitstick._state[i];

    return *this;
}

Bitstick32 constexpr operator- (const Bitstick32& rhsBitstick) const
{
    std::uint32_t result;

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

bool constexpr operator[] (const int index) const
{
    return _state[index / 64ull] & (1ull << (index % 64u));
}

bool constexpr operator== (const Bitstick32& rhs) const
{
    return is_equal(&_state[0], &_state[WordCount], &rhs._state[0]);
}

bool constexpr operator!= (const Bitstick32& rhs) const
{
    return isnt_equal(&_state[0], &_state[WordCount], &rhs._state[0]);
}

explicit constexpr operator bool()const
{
    const Bitstick32 nil;
    return isnt_equal(&_state[0], &_state[WordCount], &nil._state[0]);
}

constexpr bool isEmpty()const
{
    const Bitstick32 nil;
    return is_equal(&_state[0], &_state[WordCount], &nil._state[0]);
}

Bitstick32 begin()const
{
    return *this;
}

Bitstick32 end()const
{
    const Bitstick32 a;
    return a;
}

friend std::ostream& operator<<(std::ostream& out, const Bitstick32<N>& bits)
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

private:

std::uint32_t _state;

};
