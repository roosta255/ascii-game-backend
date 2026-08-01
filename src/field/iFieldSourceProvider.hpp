#pragma once
#include <cstdint>

class Match;

class iFieldSourceProvider {
public:
    virtual int16_t getRoomBaseValue(const Match& match, int roomId) const = 0;
    virtual ~iFieldSourceProvider() = default;
};
