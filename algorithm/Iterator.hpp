#pragma once

#include <ostream>
#include <vector>

struct Iterator
{

const int _begin, _current, _end;
 
constexpr Iterator
( const int hi
)
: _begin(0)
, _current(0)
, _end(hi)
{
}

constexpr Iterator
( const int lo
, const int hi
)
: _begin(lo)
, _current(lo)
, _end(hi)
{
}

constexpr Iterator
( const int lo
, const int current
, const int hi
)
: _begin(lo)
, _current(current)
, _end(hi)
{
}

Iterator(const Iterator&) = default;

Iterator& operator=(const Iterator&);

Iterator& operator++();
Iterator operator++(int);

int constexpr operator*()const
{
    return _current;
}

bool constexpr operator==(const Iterator& rhs)const
{
    return _current == rhs._current
        && _begin == rhs._begin
        && _end == rhs._end;
}

bool constexpr operator!=(const Iterator& rhs)const
{
    return _current != rhs._current
        || _begin != rhs._begin
        || _end != rhs._end;
}

bool constexpr isSized()const
{
    return _begin >= _end;
}

int constexpr size()const
{
    return _end - _begin;
}

Iterator constexpr shrink()const
{
    return Iterator(_begin + 1, _begin + 1, _end - 1);
}

Iterator constexpr grow()const
{
    return Iterator(_begin - 1, _begin - 1, _end + 1);
}

Iterator begin()const
{
    return Iterator(_begin, _begin, _end);
}

Iterator end()const
{
    return Iterator(_begin, _end, _end);
}

friend
std::ostream& operator<<(std::ostream&, const Iterator&);

};

