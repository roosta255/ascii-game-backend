#pragma once

#include <functional>

#define HASH_MACRO_DECL(TYPENAME) \
namespace std { \
    template<> \
    struct hash<TYPENAME> { \
        size_t operator()(const TYPENAME& m) const noexcept { \
            const unsigned char* p = reinterpret_cast<const unsigned char*>(&m); \
            size_t offset_basis = 2166136261u; \
            constexpr auto FNV_prime = 16777619u; \
            for (size_t i = 0; i < sizeof(TYPENAME); ++i) { \
                offset_basis ^= p[i]; \
                offset_basis *= FNV_prime; \
            } \
            return offset_basis; \
        } \
    }; \
}

