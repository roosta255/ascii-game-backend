#pragma once

// Class for hacking off const
template<typename T>
union Address {
    T* raw;
    const T* immutable;
    constexpr Address(const T* init): immutable(init) {}
};
