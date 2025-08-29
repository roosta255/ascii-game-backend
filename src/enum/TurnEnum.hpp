#pragma once

#include <string>

#define TURN_DECL( name ) TURN_##name,
enum TurnEnum
{
#include "Turn.enum"
TURN_COUNT
};
#undef TURN_DECL

const char* turn_to_text(int);
bool text_to_turn(const std::string& text, TurnEnum& output);

inline std::ostream& operator<<(std::ostream& os, TurnEnum turn) {
    return os << std::string(turn_to_text(turn));
}