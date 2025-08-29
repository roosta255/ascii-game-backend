#pragma once

#include <climits>

template<typename T>
constexpr size_t confine(size_t length)
{
 return (sizeof(T) * 8 - 1) & length;
}

static_assert(confine<unsigned>(36) == 4, "");
static_assert(confine<unsigned long long>(68) == 4, "");

template <typename INT>
INT constexpr bit_roll(INT val, size_t len) 
{
#if __cplusplus > 201100L && _wp_force_unsigned_rotate // Apply unsigned check C++ 11 to make sense
    static_assert(std::is_unsigned<INT>::value,
                  "Rotate Left only makes sense for unsigned types");
#endif
    return (val << confine<INT>(len)) | ((unsigned) val >> (-confine<INT>(len) & (sizeof(INT) * CHAR_BIT - 1)));
}

static_assert(bit_roll<unsigned>(0xff, 0) == 0xff, "");
static_assert(bit_roll<unsigned>(0xff, 4) == 0xff0, "");
static_assert(bit_roll<unsigned>(0xff, 36) == 0xff0, "");
static_assert(bit_roll<unsigned>(0xff000000, 4) == 0xf000000f, "");
static_assert(bit_roll<unsigned>(0xff000000, 36) == 0xf000000f, "");
