#pragma once

#include <ostream>
#include <string>

#define CONDUCT_DECL(name_) CONDUCT_##name_,
enum ConductEnum {
#include "Conduct.enum"
    CONDUCT_COUNT
};
#undef CONDUCT_DECL

const char* conduct_to_text(int);

inline std::ostream& operator<<(std::ostream& os, const ConductEnum& code) {
    return os << std::string(conduct_to_text(code));
}
