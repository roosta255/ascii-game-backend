#pragma once

#include <array>
#include <iterator>
#include <functional>

#include "Cardinal.hpp"
#include "vector/int2.hpp"

template<typename T>
class Compass
{
	std::array<T, 4> data;

public:

	constexpr
	Compass(const T north, const T east, const T south, const T west): data{north, east, south, west}{}

	typename std::array<T, 4>::iterator begin()
	{
		return data.begin();
	}

	typename std::array<T, 4>::iterator end()
	{
		return data.end();
	}

	typename std::array<T, 4>::const_iterator begin()const
	{
		return data.begin();
	}

	typename std::array<T, 4>::const_iterator end()const
	{
		return data.end();
	}

	T& operator[](const Cardinal index)
	{
		return data[index.getIndex() % 4];
	}

	constexpr const T& operator[](const Cardinal index)const
	{
		return data[index.getIndex() % 4];
	}

	friend
	std::ostream& operator<<(std::ostream& out,
						const Compass& model)
	{
	out << model.data;

	return out;
	}

};
