#pragma once

#include <utility>
#include <cstdint>
#include "data/Bitstick.hpp"
// Nodus Tollens
// n.
// the realization that the plot of your life
// doesn’t make sense to you anymore—
// that although you thought you were following the arc of the story,
// you keep finding yourself immersed in passages you don’t understand,
// that don’t even seem to belong in the same genre—
// which requires you to go back
// and reread the chapters you had originally skimmed to get to the good parts,
// only to learn that all along you were supposed to choose your own adventure.

class Pcg
{

typedef std::uint64_t Bits;

Bits _state, _inc;

public:

Pcg();

Pcg
( const Bits state
, const Bits inc
);

static Pcg getNow();

Bits tick();
std::uint64_t tickAnd(std::uint64_t);
std::uint64_t tickBit(std::uint64_t);
std::uint64_t tickOne(std::uint64_t);

bool Pcg::findOneOnBit
( const std::uint64_t bits
, std::uint64_t& out
);

template<size_t N>
bool indexofBit(const Bitstick<N>& bits, int& out)
{
    if(bits.isEmpty())
        return false;
    
    return indexofBit(&bits[0], Bitstick<N>::WordCount, Bitstick<N>::LastMask, out);
}

bool indexofBit(const Bitline*, Bitline cntWord, Bitline maskLast, int& out);

Pcg skip(const std::size_t iterations = 1)const;

Bits pickParameter(const Bits a, const Bits b);

Bits pickParameter(const Bits a, const Bits b, const Bits, const Bits);

Bits pickArray(const Bits params[], int count);

};
