#pragma once

#include <string>
#include "CodeType.hpp"

#define MOVEMENT_DECL( name, type ) CODE_##name,
enum CodeEnum
{
#include "Code.enum"
CODE_COUNT
};
#undef MOVEMENT_DECL

const char* code_to_text(int);
const CodeType code_to_type(int);
const std::string code_to_message(int code, const std::string prefix = "");

inline std::ostream& operator<<(std::ostream& os, CodeEnum code) {
    return os << std::string(code_to_text(code));
}
