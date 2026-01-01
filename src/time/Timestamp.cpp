#include "Timestamp.hpp"

#include <chrono>

Timestamp::Timestamp():
mTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()).count()){}
