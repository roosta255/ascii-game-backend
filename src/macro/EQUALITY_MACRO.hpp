#pragma once

#include <cstring>

#define EQUALITY_DECL(TYPENAME) \
bool operator==(const TYPENAME& other) const


#define EQUALITY_DEF(TYPENAME) \
bool TYPENAME::operator==(const TYPENAME& other) const { \
    return std::memcmp(this, &other, sizeof(TYPENAME)) == 0; \
}
