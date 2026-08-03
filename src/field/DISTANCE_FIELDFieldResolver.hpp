#pragma once
#include "iFieldResolver.hpp"

class DISTANCE_FIELDFieldResolver : public iFieldResolver {
public:
    void resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const override;
};
