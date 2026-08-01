#pragma once
#include "iFieldResolver.hpp"

class ROOM_ACCUMULATIONFieldResolver : public iFieldResolver {
public:
    void resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const override;
};
