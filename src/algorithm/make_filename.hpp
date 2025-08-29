#pragma once

#include <string>
#include <chrono>

inline std::string make_filename(const std::string& prefix)
{
    auto now = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return prefix + "-" + std::to_string(epoch);
}