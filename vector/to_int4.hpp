#pragma once

#include "int2.hpp"
#include "int3.hpp"
#include "int4.hpp"

constexpr int4 to_int4(const int2 ints, int z = 0, int t = 0)
{
	return int4{ints[0], ints[1], z, t};
}

int4 constexpr to_int4(const int3 ints, int t)
{
	return int4{ints[0], ints[1], ints[2], t};
}
