#pragma once

#include <string>

class Timestamp
{

long mTime;
constexpr Timestamp(long time): mTime(time){}

public:

static constexpr Timestamp buildTimestamp(long time) { return Timestamp(time); }

Timestamp();

constexpr operator long()const
{ return mTime; }

constexpr bool operator==(const Timestamp& other)const
{ return mTime == other.mTime; }

constexpr bool operator!=(const Timestamp& other)const
{ return mTime != other.mTime; }

constexpr Timestamp operator-(const long other)const
{ return Timestamp(mTime - other); }

constexpr Timestamp operator+(const long other)const
{ return Timestamp(mTime + other); }

constexpr bool operator<=(const Timestamp rhs)const{ return mTime <= rhs.mTime;}
constexpr bool operator<(const Timestamp rhs)const{ return mTime < rhs.mTime;};
constexpr bool operator>(const Timestamp rhs)const{ return mTime > rhs.mTime;}
constexpr bool operator>=(const Timestamp rhs)const{ return mTime >= rhs.mTime;}

bool constexpr isNil()const{ return 0L == mTime; }
static constexpr Timestamp nil(){return Timestamp(0L);};

};
