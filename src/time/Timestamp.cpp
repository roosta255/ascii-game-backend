#include "Timestamp.hpp"

#include <chrono>

Timestamp::Timestamp(){
    const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()).count();
    const auto copy = Timestamp::fromLong(time);
    mHigh = copy.mHigh;
    mLow = copy.mLow;
}