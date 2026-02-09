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

constexpr bool isEmpty()const{
	return !_exists;
}

constexpr bool isPresent()const{
	return _exists;
}

template<typename F>
void access(F f){
	if(isPresent())
		f(_data);
}

template<typename F>
void accessConst(F f)const{
	if(isPresent())
		f(_data);
}

template<typename T2, typename F>
Maybe<T2> map(F f){
    if(isPresent())
        return Maybe<T2>(f(_data));
	return Maybe<T2>();
}

inline bool copy(T& output)const{
	if (isPresent()) {
		output = _data;
		return true;
	}
	return false;
}

inline Maybe<T>& operator=(const T& setting){
	_data = setting;
	_exists = true;
	return *this;
}

inline bool operator==(const T& rhs)const{
	return isPresent() && _data == rhs;
}

inline bool operator==(const Maybe<T>& rhs)const{
	return isPresent() ? rhs.isPresent() && _data == rhs._data : rhs.isEmpty();
}

inline const T& orElse(const T& defau1t)const{
	return isPresent()
    ? _data : defau1t;
}

private:

T _data;
bool _exists;

};

