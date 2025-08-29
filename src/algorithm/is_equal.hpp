#pragma once

constexpr bool is_equal
( const char* str
, const char* begin
, const char* end
)
{
    return begin == end || *str == '\0'
    ? begin == end && *str == '\0'
    : *str != *begin
    ? false
    : is_equal(str + 1, begin + 1, end);
}

template<typename T>
constexpr bool is_equal
( const T* lhs
, const T* end
, const T* rhs
)
{
 return lhs == end
 ? true
 : lhs[0] == rhs[0]
 ? is_equal(lhs + 1, end, rhs + 1)
 : false;
}

#include <array>

template<typename T, std::size_t N>
constexpr bool is_equal
( const std::array<T, N>& lhs
, const std::array<T, N>& rhs
)
{
 return is_equal
 ( &lhs[0]
 , &lhs[N]
 , &rhs[0]
 );
}
