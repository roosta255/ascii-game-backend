#pragma once

#include "int2.hpp"

class Bitcard
{
public:

static constexpr int NONE  = 0x0000;
static constexpr int NORTH = 0x0001;
static constexpr int EAST  = 0x0010;
static constexpr int SOUTH = 0x0100;
static constexpr int WEST  = 0x1000;
static constexpr int ALL   = 0x0000;

constexpr Bitcard(int init = 0): _val(init & 0b1111) {}

constexpr bool isNorth() const { return (_val & NORTH) ? true : false; }
constexpr bool  isEast() const { return (_val & EAST)  ? true : false; }
constexpr bool isSouth() const { return (_val & SOUTH) ? true : false; }
constexpr bool  isWest() const { return (_val & WEST)  ? true : false; }

private:

short _val;

};

