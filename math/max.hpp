#pragma once

#include <array>

template<typename T> constexpr T max(const T& l, const T& r)
{
    return l < r ? r : l;
}

template<typename T> constexpr std::array<T, 2> max(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{ max(l[0], r[0]), max(l[1], r[1]) }; }
    
    
template<typename T> constexpr std::array<T, 3> max(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{ max(l[0], r[0]), max(l[1], r[1]), max(l[2], r[2]) }; }
    
    
template<typename T> constexpr std::array<T, 4> max(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{ max(l[0], r[0]), max(l[1], r[1]), max(l[2], r[2]), max(l[3], r[3]) }; }
