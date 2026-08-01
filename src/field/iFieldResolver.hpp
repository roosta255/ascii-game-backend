#pragma once

class Match;
class iFieldSourceProvider;
struct FieldFlyweight;
struct FieldState;

class iFieldResolver {
public:
    virtual void resolve(const Match& match, const iFieldSourceProvider& source, const FieldFlyweight& flyweight, FieldState& state) const = 0;
    virtual ~iFieldResolver() = default;
};
