#pragma once

#include "vector/int2.hpp"

class Cardinal
{
public:

constexpr Cardinal(int init)
: _val(init % 4) {}

explicit constexpr operator int(){
	return _val;
}

static constexpr Cardinal north(){
	return Cardinal(0);
}

static constexpr Cardinal south(){
	return Cardinal(2);
}

static constexpr Cardinal east(){
	return Cardinal(1);
}

static constexpr Cardinal west(){
	return Cardinal(3);
}

static constexpr std::array<int2, 4> getRectOffsets() {
	return std::array<int2, 4>
	{ int2{0,1}
    , int2{1,0}
    , int2{0,-1}
    , int2{-1,0}
	};
}

static constexpr std::array<Cardinal, 4> getAllCardinals() {
	return std::array<Cardinal, 4>
	{ Cardinal(0)
    , Cardinal(1)
    , Cardinal(2)
    , Cardinal(3)
	};
}

// 0, 1, 2, 3
// 0, 1, 0, -1
// 1  2  3  4
// 1  0     0
// 1  0 -1  0

static constexpr int sine(int i){
	return (i & 1) - (i == 3 ? 2 : 0);
}

static constexpr int cosine(int i){
	return ((i + 1) & 1) - (i == 2 ? 2 : 0);
}

constexpr int2 getRectOffset()const{
	return int2{sine(_val), cosine(_val)};
}

constexpr Cardinal getFlip()const{
	return Cardinal(_val + 2);
}

constexpr Cardinal getClockwise()const{
	return Cardinal(_val + 1);
}

constexpr Cardinal getCounterClockwise()const{
	return Cardinal(_val + 3);
}

constexpr int getIndex() const {
	return _val;
}

private:

short _val;

};

static_assert(Cardinal::north().getRectOffset() == int2{0,1}, "");
static_assert(Cardinal::east().getRectOffset() == int2{1,0}, "");
static_assert(Cardinal::south().getRectOffset() == int2{0,-1}, "");
static_assert(Cardinal::west().getRectOffset() == int2{-1,0}, "");




