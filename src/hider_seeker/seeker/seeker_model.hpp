#pragma once

#include <cstdint>
#include <string>
#include "Array.hpp"

namespace hider_seeker {

constexpr int SEEKER_MODEL_INPUTS = 43;
constexpr int SEEKER_MODEL_HIDDEN1 = 32;
constexpr int SEEKER_MODEL_HIDDEN2 = 16;
constexpr int SEEKER_MODEL_OUTPUTS = 3;
constexpr size_t SEEKER_MODEL_BYTES = 4076;

struct SeekerModel {
    Array<Array<int16_t, SEEKER_MODEL_INPUTS>, SEEKER_MODEL_HIDDEN1> w1;
    Array<int32_t, SEEKER_MODEL_HIDDEN1> b1;
    Array<Array<int16_t, SEEKER_MODEL_HIDDEN1>, SEEKER_MODEL_HIDDEN2> w2;
    Array<int32_t, SEEKER_MODEL_HIDDEN2> b2;
    Array<Array<int16_t, SEEKER_MODEL_HIDDEN2>, SEEKER_MODEL_OUTPUTS> w3;
    Array<int32_t, SEEKER_MODEL_OUTPUTS> b3;
};

int64_t floorDiv(int64_t n, int64_t d);
void runSeekerModel(const SeekerModel& model, const Array<int16_t, SEEKER_MODEL_INPUTS>& input, Array<int32_t, SEEKER_MODEL_OUTPUTS>& output);
SeekerModel makeRandomSeekerModel(uint64_t seed);
std::string seekerModelBytes(const SeekerModel& model);
bool seekerModelFromBytes(const std::string& bytes, SeekerModel& model);

} // namespace hider_seeker