#pragma once
#include "iFieldResolver.hpp"

class RELAXATIONFieldResolver : public iFieldResolver {
public:
    void resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const override;
};
