#pragma once

template<typename T>
class Maybe
{
public:

constexpr Maybe(const T& init)
: _data(init)
, _exists(true){}

constexpr Maybe()
: _data()
, _exists(false){}

constexpr static Maybe empty(){
	return Maybe();
}

constexpr bool isEmpty(){
	return !_exists;
}

constexpr bool isPresent(){
	return _exists;
}

template<typename F>
void access(F f){
	if(isPresent())
		f(*_data);
}

template<typename T2, typename F>
Maybe<T2> map(F f){
    if(isPresent())
        return Maybe<T2>(f(_data));
	return Maybe<T2>();
}

inline Maybe<T>& operator=(const T& setting){
	_data = setting;
	return *this;
}

inline bool operator==(const T& rhs){
	return _data == rhs;
}

inline const T& orElse(const T& defau1t){
	return isPresent()
    ? _data : defau1t;
}

private:

T _data;
bool _exists;

};

