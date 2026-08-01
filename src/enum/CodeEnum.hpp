#pragma once

#include <string>
#include "CodeType.hpp"
#include "ConductMemoryVariableEnum.hpp"

#define MOVEMENT_DECL( name, type, valueType ) CODE_##name,
enum CodeEnum
{
#include "Code.enum"
CODE_COUNT
};
#undef MOVEMENT_DECL

const char* code_to_text(int);
const CodeType code_to_type(int);
ConductMemoryVariableTypeEnum code_to_value_type(int);
const std::string code_to_message(int code, const std::string prefix = "");
const std::string code_to_value_text(int code, int value);
bool code_from_text(const std::string& name, CodeEnum& output);

inline std::ostream& operator<<(std::ostream& os, CodeEnum code) {
    return os << std::string(code_to_text(code));
}
