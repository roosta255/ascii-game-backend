#include "Array.hpp"
#include <iostream>
#include <array>
#include <numeric>   // for std::iota
#include <algorithm> // for std::shuffle
#include <random>    // for std::mt19937

template <std::size_t N>
Array<int, N> generateRandomArray(unsigned int seed) {
    std::array<int, N> data;

    // 1. Fill the array with values 0, 1, 2, ..., N-1
    std::iota(data.begin(), data.end(), 0);

    // 2. Initialize the random number generator with the seed
    std::mt19937 g(seed);

    // 3. Shuffle the array
    std::shuffle(data.begin(), data.end(), g);

    return Array<int, N>(data);
}
