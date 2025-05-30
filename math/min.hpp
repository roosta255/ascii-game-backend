#pragma once

#include <array>

template<typename T> constexpr T min(const T& l, const T& r)
{
    return l < r ? l : r;
}

template<typename T> constexpr std::array<T, 2> min(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{ min(l[0], r[0]), min(l[1], r[1]) }; }
template<typename T> constexpr std::array<T, 3> min(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{ min(l[0], r[0]), min(l[1], r[1]), min(l[2], r[2]) }; }
template<typename T> constexpr std::array<T, 4> min(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{ min(l[0], r[0]), min(l[1], r[1]), min(l[2], r[2]), min(l[3], r[3]) }; }
