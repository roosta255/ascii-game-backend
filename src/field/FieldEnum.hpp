#pragma once
#include <string>

#define FIELD_DECL(name_, resolver_, provider_, mask_, min_, max_) FIELD_##name_,
enum FieldEnum
{
#include "Field.enum"
FIELD_COUNT
};
#undef FIELD_DECL

const char* field_to_text(const FieldEnum&);

inline std::ostream& operator<<(std::ostream& os, FieldEnum field) {
    return os << std::string(field_to_text(field));
}
