#pragma once

#include <string>

class Timestamp
{
    // Split the 8-byte long into two 4-byte pieces
    // This changes alignment from 8 to 4
    int32_t mHigh; 
    int32_t mLow;

    constexpr Timestamp(int32_t high, int32_t low) 
        : mHigh(high), mLow(low) {}

    // Internal helper to reconstruct the 64-bit value for math
    constexpr int64_t total() const {
        return (static_cast<int64_t>(mHigh) << 32) | (static_cast<uint32_t>(mLow));
    }

    static constexpr Timestamp fromLong(int64_t time) {
        return Timestamp(
            static_cast<int32_t>(time >> 32),
            static_cast<int32_t>(time & 0xFFFFFFFF)
        );
    }

public:
    static constexpr Timestamp buildTimestamp(int64_t time) { 
        return fromLong(time); 
    }

    Timestamp();

    // Keep the operator long for compatibility
    constexpr operator int64_t() const { return total(); }

    constexpr bool operator==(const Timestamp& other) const {
        return mHigh == other.mHigh && mLow == other.mLow;
    }

    constexpr bool operator!=(const Timestamp& other) const {
        return !(*this == other);
    }

    constexpr Timestamp operator-(const int64_t other) const {
        return fromLong(total() - other);
    }

    constexpr Timestamp operator+(const int64_t other) const {
        return fromLong(total() + other);
    }

    // Comparison operators use the reconstructed total
    constexpr bool operator<=(const Timestamp rhs) const { return total() <= rhs.total(); }
    constexpr bool operator<(const Timestamp rhs) const { return total() < rhs.total(); }
    constexpr bool operator>(const Timestamp rhs) const { return total() > rhs.total(); }
    constexpr bool operator>=(const Timestamp rhs) const { return total() >= rhs.total(); }

    constexpr bool isNil() const { return mHigh == 0 && mLow == 0; }
    static constexpr Timestamp nil() { return Timestamp(0, 0); }
};
