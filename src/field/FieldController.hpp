#pragma once
#include <cstdint>
#include "Array.hpp"
#include "FieldEnum.hpp"
#include "FieldState.hpp"

class Match;

struct FieldController {
    void recompute(const Match& match);
    int16_t get(FieldEnum field, int roomId) const;

private:
    Array<FieldState, FIELD_COUNT> states;
};
