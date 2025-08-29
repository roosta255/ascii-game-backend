#pragma once

#include <algorithm>
#include <array>

template<typename T> constexpr T clamp(const T& v, const T& lo, const T& hi) {
    return v < lo ? lo : hi < v ? hi : v;
}

template<typename T> constexpr std::array<T, 2> clamp(const std::array<T, 2>& v, const std::array<T, 2>& lo, const std::array<T, 2>& hi) {
    return std::array<T, 2>{ clamp(v[0], lo[0], hi[0]), clamp(v[1], lo[1], hi[1]) }; }
template<typename T> constexpr std::array<T, 3> clamp(const std::array<T, 3>& v, const std::array<T, 3>& lo, const std::array<T, 3>& hi) {
    return std::array<T, 3>{ clamp(v[0], lo[0], hi[0]), clamp(v[1], lo[1], hi[1]), clamp(v[2], lo[2], hi[2]) }; }
template<typename T> constexpr std::array<T, 4> clamp(const std::array<T, 4>& v, const std::array<T, 4>& lo, const std::array<T, 4>& hi) {
    return std::array<T, 4>{ clamp(v[0], lo[0], hi[0]), clamp(v[1], lo[1], hi[1]), clamp(v[2], lo[2], hi[2]), clamp(v[3], lo[3], hi[3]) }; }
