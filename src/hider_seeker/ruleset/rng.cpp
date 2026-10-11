#include "rng.hpp"
#include <cstdint>

namespace hider_seeker {

// Simple deterministic integer PRNG (splitmix64-style)
// This is a placeholder implementation - a real implementation would use a proper PRNG
uint32_t nextRandomValue(RngState& rng) {
    // SplitMix64 algorithm for generating pseudo-random numbers
    uint64_t z = (rng.state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    rng.counter++;
    
    // Return lower 32 bits
    return static_cast<uint32_t>(z >> 32);
}

} // namespace hider_seeker