#pragma once

#include <array>
#include <string>

#include "data/Rack.hpp"
#include "rng/Pcg.hpp"
#include "rng/twist.hpp"

//motivation:
//***********************************
//									*
//	map<Uuid, object> seems cool	*
//									*
//***********************************

// Liberosis
// n.
// the desire to care less about things—
// to loosen your grip on your life,
// to stop glancing behind you every few steps,
// afraid that someone will snatch it from you
// before you reach the end zone—
// rather to hold your life loosely and playfully,
// like a volleyball, keeping it in the air,
// with only quick fleeting interventions,
// bouncing freely in the hands of trusted friends,
// always in play.

struct Uuid
{
const std::array<std::uint64_t, 2> bits;

constexpr
Uuid(const std::uint64_t lhs,
     const std::uint64_t rhs):
     bits{lhs, rhs}{}

Uuid();

Uuid(const std::string& uuid);

Uuid& operator=(const Uuid&);

std::string toString() const;

std::string toString(const int i)const;

operator std::string()const
{
	return toString();
}

Uuid roll(const unsigned int)const;

Pcg getPcg()const;

constexpr bool isntNil()const
{
    return operator!=(getNil());
}

constexpr bool isNil()const
{
    return operator==(getNil());
}

static constexpr Uuid getNil()
{
 return Uuid(0ul,0ul);
}

static inline constexpr
Uuid nil()
{
 return Uuid(0ul,0ul);
}

constexpr
bool operator==(const Uuid uuid)const
{
	return bits[0] == uuid.bits[0]
		&& bits[1] == uuid.bits[1];
}

constexpr
bool operator!= (const Uuid uuid) const
{
	return bits[0] != uuid.bits[0]
		|| bits[1] != uuid.bits[1];
}

bool operator<(const Uuid other)const;

constexpr
const Uuid getUuid() const
{
	return *this;
}

constexpr
static Uuid getAlpha()
{
   return Uuid(0x6e344693848f2094,
			   0xb4db73a42574c313);
}

constexpr
static Uuid hash(const char* str,
                 const Uuid uuid = getAlpha())
{
   //static_assert(hash("") == getAlpha());
   return *str ? uuid
               : hash(str + 1, uuid);
}

unsigned search(const Rack<Uuid>,
                const Uuid)const;

};

std::istream& operator>>(std::istream& in,
					Uuid& uuid);

std::ostream& operator<<(std::ostream& out,
					const Uuid uuid);

template<>
struct std::hash<Uuid>
{
    std::size_t operator()(const Uuid& bits)const
    {
        return twist(bits.bits[0], bits.bits[1]);
    }
};

template<>
struct std::equal_to<Uuid>
{
    bool operator()(const Uuid& lhs, const Uuid& rhs)const
    {
        return lhs == rhs;
    }
};
