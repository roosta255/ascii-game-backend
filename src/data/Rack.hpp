#pragma once
#include <array>
#include <cstring>
#include <experimental/optional>
#include <memory>
#include <vector>

#include "is_equal.hpp"
#include "Address.hpp"
#include "Array.hpp"
// #include "data/Bake.hpp"
// #include "data/Cake.hpp"
#include "Iterator.hpp"
#include "Pointer.hpp"

template<typename T>
class Rack
{
T* start;
T* occupied;
T* finish;

constexpr Rack
( T* start
, T* occupied
, T* finish
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
: start(Address(arr.data()).raw)
, finish(Address(arr.data()).raw + N)
, occupied(Address(arr.data()).raw + N)
{
}

template<unsigned N>
constexpr Rack
(const Array<T, N>& arr) noexcept
: start(Address(arr.begin()).raw)
, finish(Address(arr.end()).raw)
, occupied(Address(arr.end()).raw)
{
}

template<long unsigned N>
constexpr static Rack<T> buildFromArray
(Array<T, N>& arr) noexcept
{
    return Rack<T>(arr.begin(), arr.end(), arr.end());
}

Pointer<T> getPointer
( const int index
)
{
    return contains(index)
    ? Pointer<T>(this->start[index])
    : Pointer<T>();
}

Pointer<const T> getPointer
( const int index
) const
{
    return contains(index)
    ? Pointer<const T>(this->start[index])
    : Pointer<const T>();
}

Rack(const std::vector<T>& arr)
: start(Address(arr.data()).raw)
, finish(Address(arr.data()).raw + arr.size())
, occupied(Address(arr.data()).raw + arr.size())
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
                start + (count < size() ? count : size()),
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

constexpr const T* begin() const
{
    return start;
}

constexpr const T* end() const
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

// Transform without index
template<typename Func>
auto transform(Func func) const -> std::vector<decltype(func(std::declval<T>()))> {
    using ResultType = decltype(func(std::declval<T>()));
    std::vector<ResultType> result;
    result.reserve(size()); // avoid reallocations
    for (auto it = start; it != occupied; ++it) {
        result.push_back(func(*it));
    }
    return result;
}

// Transform with index
template<typename Func>
auto transform(Func func) const -> std::vector<decltype(func(int{}, std::declval<T>()))> {
    using ResultType = decltype(func(int{}, std::declval<T>()));
    std::vector<ResultType> result;
    result.reserve(size());
    int i = 0;
    for (auto it = start; it != occupied; ++it, ++i) {
        result.push_back(func(i, *it));
    }
    return result;
}

};
