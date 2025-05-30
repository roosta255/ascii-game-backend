#pragma once

#include "data/Maybe.hpp"

template<typename T>
class Pointer
{

public:

constexpr Pointer(T& init)
: _ptr(&init){}

constexpr Pointer()
: _ptr(nullptr){}

constexpr static Pointer empty(){
	return Pointer();
}

constexpr bool isEmpty(){
	return nullptr == _ptr;
}

constexpr bool isPresent() const {
	return nullptr != _ptr;
}

template<typename F>
bool access(F f){
	if (isPresent()) {
		f(*_ptr);
		return true;
	}
	return false;
}

template<typename F>
bool accessConst(F f) const {
	if (isPresent()) {
		f(*_ptr);
		return true;
	}
	return false;
}

template<typename T2, typename F>
Maybe<T2> map(F f){
    if(isPresent())
        return Maybe<T2>(f(*_ptr));
	return Maybe<T2>();
}

inline Pointer<T>& operator=(T& setting){
	_ptr = &setting;
	return *this;
}

inline bool operator==(T& rhs){
	return _ptr == &rhs;
}

inline T& orElse(T& defau1t){
	return isPresent()
    ? *_ptr : defau1t;
}

inline T* getAddress(){
	return _ptr;
}

private:

T* _ptr = nullptr;

};

