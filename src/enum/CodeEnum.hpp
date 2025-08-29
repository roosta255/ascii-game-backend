#pragma once

#include <string>

#define MOVEMENT_DECL( name ) CODE_##name,
enum CodeEnum
{
#include "Code.enum"
CODE_COUNT
};
#undef MOVEMENT_DECL

const char* code_to_text(int);
const std::string code_to_message(int code, const std::string prefix = "");

inline std::ostream& operator<<(std::ostream& os, CodeEnum code) {
    return os << std::string(code_to_text(code));
}
