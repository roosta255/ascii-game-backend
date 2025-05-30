#pragma once

#include <array>
#include <iterator>
#include <functional>

#include "vector/int2.hpp"

template<typename T>
class Quad
{
	
std::array<T, 4> myCorners;

public:

enum {
	eTopRight,
	eTopLeft,
	eBotLeft,
	eBotRight,
	eCorners
};

//constexpr Quad(const array<TYPE, 4> & corners) : myCorners(corners) {}

constexpr
Quad(){}

constexpr
Quad(const T topRight,
	 const T topLeft,
	 const T botLeft,
	 const T botRight):
myCorners{topRight,
		  topLeft,
		  botLeft,
		  botRight}{}

constexpr
Quad(const Quad& copy)
: myCorners(copy.myCorners)
{}

constexpr
Quad(const T input):
myCorners{input,
		  input,
		  input,
		  input}{}

//constexpr Quad(TYPE topRight, TYPE topLeft, TYPE botLeft, TYPE botRight) :
//	myCorners{topRight, topLeft, botLeft, botRight} {}

constexpr
Quad flipHorizontal()const
{
	return Quad<T>(topLeft(), 
	               topRight(), 
				   botRight(), 
				   botLeft());
}

constexpr
Quad flipVertical()const
{
	return Quad<T>(botRight(),
					  botLeft(),
					  topLeft(),
					  topRight());
}

static
T lerp(T a, float r, T b)
{
	return a + (b - a) * r;
}

T lerp(float x, float y)const
{
   return lerp(
       lerp(topLeft(), x, topRight()),
	           y,
       lerp(botLeft(), x, botRight()));
}

Quad getGrid(int2 xy, int2 grid)const
{
	float xRatio  = (float)(xy[0]) / grid[0];
	float xRatio2 = (float)(xy[0] + 1) / grid[0];
	
	T topleft  = lerp(topLeft(),
                      xRatio,
					  topRight());
	T topright = lerp(topLeft(),
                      xRatio2,
					  topRight());
	T botleft  = lerp(botLeft(),
	                  xRatio,
					  botRight());
	T botright = lerp(botLeft(),
                      xRatio2,
					  botRight());
	
	float yRatio  = (float)(xy[1]) / grid[1];
	float yRatio2 = (float)(xy[1] + 1) / grid[1];
	//const T topRight,
	//const T topLeft,
	//const T botLeft,
	//const T botRight
	return Quad<T>(
	         lerp(topright,
			      yRatio,
			      botright),
			 lerp(topleft,
			      yRatio,
			      botleft),
			 lerp(topleft,
			      yRatio2,
			      botleft),
			 lerp(topright,
			      yRatio2,
			      botright)
	);
}

constexpr
Quad turnClockwise()const
{
	return Quad<T>(topLeft(),
				   botLeft(),
				   botRight(),
				   topRight());
}

constexpr
Quad turnCounterclockwise()const
{
	return Quad<T>(botRight(),
				   topRight(),
				   topLeft(),
				   botLeft());
}

T& topRight()
{
	return myCorners[eTopRight];
}

constexpr T topRight()const
{
	return myCorners[eTopRight];
}

T& topLeft()
{
	return myCorners[eTopLeft];
}

constexpr
T topLeft()const
{
	return myCorners[eTopLeft];
}

T& botRight()
{
	return myCorners[eBotRight];
}

constexpr
T botRight()const
{
	return myCorners[eBotRight];
}

T& botLeft()
{
	return myCorners[eBotLeft];
}

constexpr
T botLeft()const
{
	return myCorners[eBotLeft];
}

typename std::array<T, 4>::iterator begin()
{
	return myCorners.begin();
}

typename std::array<T, 4>::iterator end()
{
	return myCorners.end();
}

typename std::array<T, 4>::const_iterator begin()const
{
	return myCorners.begin();
}

typename std::array<T, 4>::const_iterator end()const
{
	return myCorners.end();
}

Quad& operator=(const Quad& quad)
{
	myCorners[0] = quad.myCorners[0];
	myCorners[1] = quad.myCorners[1];
	myCorners[2] = quad.myCorners[2];
	myCorners[3] = quad.myCorners[3];
	
	return *this;
}

T& operator[](const int index)
{
	return myCorners[index];
}

constexpr const T& operator[](const int index)const
{
	return myCorners[index % 4];
}

constexpr
Quad operator* (const T& right)const
{
	return Quad(myCorners[0] * right,
				myCorners[1] * right,
				myCorners[2] * right,
				myCorners[3] * right);
}

constexpr
Quad operator+(const T& right)const
{
	return Quad(myCorners[0] + right,
				myCorners[1] + right,
				myCorners[2] + right,
				myCorners[3] + right);
}

constexpr
Quad operator&(const Quad& right)const
{
	return Quad(myCorners[0] & right[0],
				myCorners[1] & right[1],
				myCorners[2] & right[2],
				myCorners[3] & right[3]);
}

constexpr
bool operator==(const Quad& right)const
{
	return myCorners[0] == right.myCorners[0]
	    && myCorners[1] == right.myCorners[1]
        && myCorners[2] == right.myCorners[2]
        && myCorners[3] == right.myCorners[3];
}

friend
std::ostream& operator<<(std::ostream& out,
					const Quad& quad)
{
 out << quad.myCorners;

 return out;
}

const Quad<T>& foreach(std::function<void(T)> fun)const
{
	for(const auto& it: myCorners)
	{
		fun(it);
	}
	
	return *this;
}

T average()const
{
	return (myCorners[0]
	     +  myCorners[1]
	     +  myCorners[2]
		 +  myCorners[3]) / 4;
}




};


