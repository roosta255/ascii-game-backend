#pragma once

#include <algorithm>
#include <functional>

#include "data/Rack.hpp"

template<typename T1, typename T2>
class RackZipper
{
public:

RackZipper
( const Rack<T1> rack1
, const Rack<T2> rack2
)
: _rack1(rack1.begin())
, _rack2(rack2.begin())
, _size(std::min(rack1.size(), rack2.size()))
{
}

void accessConst
( std::function<void(const T1&, const T2&)> func
) const {
	for(int i = 0; i < this->_size; i++){
		func(this->_rack1[i], this->_rack2[i]);
	}
}

void access
( std::function<void(T1&, const T2&)> func
) {
	for(int i = 0; i < this->_size; i++){
		func(this->_rack1[i], this->_rack2[i]);
	}
}

private:

int _size;
T1* _rack1;
T2* _rack2;

};
