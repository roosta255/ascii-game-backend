#pragma once

#include <array>

template<typename T> constexpr std::array<T, 2> operator+(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{l[0] + r[0], l[1] + r[1]}; }
template<typename T> constexpr std::array<T, 3> operator+(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{l[0] + r[0], l[1] + r[1], l[2] + r[2]}; }
template<typename T> constexpr std::array<T, 4> operator+(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{l[0] + r[0], l[1] + r[1], l[2] + r[2], l[3] + r[3]}; }

template<typename T> constexpr std::array<T, 2> operator-(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{l[0] - r[0], l[1] - r[1]}; }
template<typename T> constexpr std::array<T, 3> operator-(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{l[0] - r[0], l[1] - r[1], l[2] - r[2]}; }
template<typename T> constexpr std::array<T, 4> operator-(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{l[0] - r[0], l[1] - r[1], l[2] - r[2], l[3] - r[3]}; }

template<typename T> constexpr std::array<T, 2> operator-(const std::array<T, 2>& r) {
    return std::array<T, 2>{ -r[0], -r[1]}; }
template<typename T> constexpr std::array<T, 3> operator-(const std::array<T, 3>& r) {
    return std::array<T, 3>{ -r[0], -r[1], -r[2]}; }
template<typename T> constexpr std::array<T, 4> operator-(const std::array<T, 4>& r) {
    return std::array<T, 4>{ -r[0], -r[1], -r[2], -r[3]}; }

template<typename T> constexpr std::array<T, 2> operator*(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{l[0] * r[0], l[1] * r[1]}; }
template<typename T> constexpr std::array<T, 3> operator*(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{l[0] * r[0], l[1] * r[1], l[2] * r[2]}; }
template<typename T> constexpr std::array<T, 4> operator*(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{l[0] * r[0], l[1] * r[1], l[2] * r[2], l[3] * r[3]}; }

template<typename T> constexpr std::array<T, 2> operator/(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{l[0] / r[0], l[1] / r[1]}; }
template<typename T> constexpr std::array<T, 3> operator/(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{l[0] / r[0], l[1] / r[1], l[2] / r[2]}; }
template<typename T> constexpr std::array<T, 4> operator/(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{l[0] / r[0], l[1] / r[1], l[2] / r[2], l[3] / r[3]}; }

template<typename T> constexpr std::array<T, 2> operator%(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return std::array<T, 2>{l[0] % r[0], l[1] % r[1]}; }
template<typename T> constexpr std::array<T, 3> operator%(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return std::array<T, 3>{l[0] % r[0], l[1] % r[1], l[2] % r[2]}; }
template<typename T> constexpr std::array<T, 4> operator%(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return std::array<T, 4>{l[0] % r[0], l[1] % r[1], l[2] % r[2], l[3] % r[3]}; }

template<typename T> constexpr std::array<T, 2> operator*(const std::array<T, 2>& l, const T& r) {
    return std::array<T, 2>{l[0] * r, l[1] * r}; }
template<typename T> constexpr std::array<T, 3> operator*(const std::array<T, 3>& l, const T& r) {
    return std::array<T, 3>{l[0] * r, l[1] * r, l[2] * r}; }
template<typename T> constexpr std::array<T, 4> operator*(const std::array<T, 4>& l, const T& r) {
    return std::array<T, 4>{l[0] * r, l[1] * r, l[2] * r, l[3] * r}; }
template<typename T> constexpr std::array<T, 2> operator/(const std::array<T, 2>& l, const T& r) {
    return std::array<T, 2>{l[0] / r, l[1] / r}; }
template<typename T> constexpr std::array<T, 3> operator/(const std::array<T, 3>& l, const T& r) {
    return std::array<T, 3>{l[0] / r, l[1] / r, l[2] / r}; }
template<typename T> constexpr std::array<T, 4> operator/(const std::array<T, 4>& l, const T& r) {
    return std::array<T, 4>{l[0] / r, l[1] / r, l[2] / r, l[3] / r}; }

template<typename T> constexpr bool operator==(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return l[0] == r[0] && l[1] == r[1]; }
template<typename T> constexpr bool operator==(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return l[0] == r[0] && l[1] == r[1] && l[2] == r[2]; }
template<typename T> constexpr bool operator==(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return l[0] == r[0] && l[1] == r[1] && l[2] == r[2] && l[3] == r[3]; }

template<typename T> constexpr bool operator!=(const std::array<T, 2>& l, const std::array<T, 2>& r) {
    return l[0] != r[0] || l[1] != r[1]; }
template<typename T> constexpr bool operator!=(const std::array<T, 3>& l, const std::array<T, 3>& r) {
    return l[0] != r[0] || l[1] != r[1] || l[2] != r[2]; }
template<typename T> constexpr bool operator!=(const std::array<T, 4>& l, const std::array<T, 4>& r) {
    return l[0] != r[0] || l[1] != r[1] || l[2] != r[2] || l[3] != r[3]; }

/*
template<typename T, size_t N>
constexpr bool operator<(const std::array<T, N> & lhs,
			   const std::array<T, N> & rhs)
{
	for(int i = 0;
		i < N;
		++i)
	{
		if (lhs[i] < rhs[i])
			continue;
		return false;
	}

	return true;
}

template<typename T, size_t N>
constexpr bool operator<=(const T & lhs,
	            const T & rhs)
{
	for (int i = 0;
		i < N;
		++i)
	{
		if (lhs[i] <= rhs[i])
			continue;
		return false;
	}

	return true;
}

template<typename T, size_t N>
constexpr bool operator>(const T & lhs,
	const T & rhs)
{
	for (int i = 0;
		i < N;
		++i)
	{
		if (lhs[i] > rhs[i])
			continue;
		return false;
	}

	return true;
}

template<typename T, size_t N>
constexpr bool operator>=(const T & lhs,
	const T & rhs)
{
	for (int i = 0;
		i < N;
		++i)
	{
		if (lhs[i] >= rhs[i])
			continue;
		return false;
	}

	return true;
}
*/
