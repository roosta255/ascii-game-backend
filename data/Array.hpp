#pragma once

#include <array>

#include "data/Pointer.hpp"

template<typename T, unsigned N>
class Array
{
public:

Array(std::initializer_list<T> initials)
: _elems{}
{
	int i = 0;
    for(const auto& entry: initials)
    {
        assignValue(i++, entry);
    }
}

T& head() {
	return _elems[0];
}

const T& head() const {
	return _elems[0];
}

constexpr Array
( const std::array<T, N>& init
) : _elems(init) {}

constexpr Array()
: _elems{} {}

constexpr int size() const {
	return N;
}

template<typename F>
bool access(const int index, F f){
	if(containsIndex(index)) {
		f(_elems[index]);
		return true;
	}
	return false;
}

inline T& getOrDefault
( const int index
, T& defauld
)
{
	return containsIndex(index)
    ? _elems[index]
    : defauld;
}

Pointer<T> getPointer
( const int index
)
{
	return containsIndex(index)
    ? Pointer<T>(_elems[index])
    : Pointer<T>();
}

template<typename F>
bool accessConst(const int index, F f) const {
	if(containsIndex(index)) {
		f(_elems[index]);
		return true;
	}
	return false;
}

inline void assignValue
( const int index
, T rhs
) {
	if(!containsIndex(index))
		return;
	_elems[index] = rhs;
}

inline void assignOutput
( const int index
, T& output
) {
	if(!containsIndex(index))
		return;
	output = _elems[index];
}

constexpr bool containsIndex
( const int index
) const {
	return index < size()
    && index >= 0;
}

constexpr bool containsAddress
( const T& elem
) const {
	return containsIndex(getAddress(elem));
}

bool containsAddress(const T& elem, int& offset)const
{
	if (containsAddress(elem)) {
        offset = &elem - &_elems[0];
        return true;
    }
    return false;
}

constexpr int getAddress
( const T& elem
) const {
	return &elem - &_elems[0];
}

constexpr T* begin()
{
    return _elems.begin();
}

constexpr T* end()
{
    return _elems.end();
}

constexpr const T* begin()const
{
    return _elems.begin();
}

constexpr const T* end()const
{
    return _elems.end();
}

private:

std::array<T, N> _elems;

};

