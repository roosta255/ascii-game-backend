#pragma once
#include <array>
#include <cstring>
#include <experimental/optional>
#include <memory>
#include <vector>

#include "is_equal.hpp"
#include "Array.hpp"
// #include "data/Bake.hpp"
// #include "data/Cake.hpp"
#include "Iterator.hpp"

template<typename T>
class Rack
{
T* start;
T* occupied;
T* finish;

constexpr Rack
( const T* start
, const T* occupied
, const T* finish
)
: start(start)
, occupied(occupied)
, finish(finish)
{
}

public:

constexpr Rack() = default;
constexpr Rack(const Rack&) = default;

constexpr Rack(const T& element) noexcept
: start(&element)
, occupied(&element + 1)
, finish(&element + 1)
{
}

template<size_t N>
constexpr Rack
(const std::array<T, N>& arr) noexcept
: start(arr.data())
, finish(arr.data() + N)
, occupied(arr.data() + N)
{
}

template<size_t N>
constexpr Rack
(const Array<T, N>& arr) noexcept
: start(arr.begin())
, finish(arr.end())
, occupied(arr.end())
{
}

Rack(const std::vector<T>& arr)
: start(arr.data())
, finish(arr.data() + arr.size())
, occupied(arr.data() + arr.size())
{
}

// Rack(const Cake<T>& cake)
// : start(cake.begin())
// , finish(cake.end())
// , occupied(cake.end())
// {
// }

template<typename T2>
Rack<T2> convert()
{
	T2* start2 = static_cast<T2>(start);
	T2* finish2 = static_cast<T2>(finish);
	T2* occupied2 = static_cast<T2>(occupied);
    Rack<T2> converted(start2, occupied2, finish2);
	return converted;
}

bool access
( const int index
, std::function<void(const T&)> func
) const
{
    if(!contains(index))
		return false;
	func(start[index]);
    return true;
}

bool access
( const int index
, std::function<void(T&)> func
)
{
    if(!contains(index))
		return false;
	func(start[index]);
    return true;
}

constexpr bool contains(const int index)const
{
	return index >= 0 && index < size();
}

void setEmpty() {
    occupied = start;
}

size_t constexpr size()const
{
    return occupied - start;
}

size_t constexpr capacity()const
{
    return finish - start;
}

bool constexpr isEmpty()const
{
    return 0 == size();
}

bool constexpr isFull()const
{
    return occupied == finish;
}

Iterator constexpr getIndices()const
{
    return Iterator(0, 0, size());
}

bool getIndex(const T& item, int& output)const
{
	output = &item - start;
	return contains(output);
}

Rack constexpr resize(const unsigned count)const
{
    return Rack(start,
                start + (count < size() ? count : size()));
}

static constexpr int min(const int a, const int b){
	return a < b ? a : b;
}
static constexpr int max(const int a, const int b){
	return a > b ? a : b;
}

Rack constexpr pop_range(const int inclusive,
               const int exclusive)const
{
	
    return exclusive <= inclusive || inclusive >= size() ? Rack()
	: Rack(start + max(inclusive, 0),
           start + min(exclusive, size()),
           start + min(exclusive, size()));
}

constexpr T* begin()
{
    return start;
}

constexpr T* end()
{
    return occupied;
}

bool constexpr operator==(const Rack<T>& rhs)
{
    return size() == rhs.size() && is_equal(begin(), end(), rhs.begin());
    //return size() == rhs.size() && is_equal(begin(), end(), rhs.begin());
}
/*
constexpr operator const T*()const
{
    return start;
}
*/
bool operator==(const std::vector<T>& rhs)const
{
    return operator==(Rack<T>(rhs));
}

friend
std::ostream& operator<<(std::ostream& out, const Rack<T> rhs)
{
    for(auto i = 0u; i < rhs.size(); i++) out << rhs[i] << ' ';

    return out;
}

Rack<T> constexpr empty()const
{
    return Rack<T>(start, start);
}

T& getOrDefault(int index, T& defaulted)
{
	return contains(index) ? start[index] : defaulted;
}

const T& getOrDefault(int index, const T& defaulted) const
{
	return contains(index) ? start[index] : defaulted;
}

};
