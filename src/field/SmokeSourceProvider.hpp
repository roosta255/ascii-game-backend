#pragma once
#include "iFieldSourceProvider.hpp"

class SmokeSourceProvider : public iFieldSourceProvider {
public:
    int16_t getRoomBaseValue(const Match& match, int roomId) const override;
};
