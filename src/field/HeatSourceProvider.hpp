#pragma once
#include "iFieldSourceProvider.hpp"

class HeatSourceProvider : public iFieldSourceProvider {
public:
    int16_t getRoomBaseValue(const Match& match, int roomId) const override;
};
