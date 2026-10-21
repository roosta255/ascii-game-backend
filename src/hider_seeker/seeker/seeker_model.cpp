#include "seeker_model.hpp"
#include "../ruleset/rng.hpp"
#include <algorithm>
#include <cstring>

namespace hider_seeker {

int64_t floorDiv(int64_t n, int64_t d) {
    if (n >= 0 || n % d == 0) {
        return n / d;
    } else {
        return n / d - 1;
    }
}

void runSeekerModel(const SeekerModel& model, const Array<int16_t, SEEKER_MODEL_INPUTS>& input, Array<int32_t, SEEKER_MODEL_OUTPUTS>& output) {
    // Layer 1: input -> hidden1
    Array<int32_t, SEEKER_MODEL_HIDDEN1> hidden1;
    
    // Process w1 weights and biases
    for (int j = 0; j < SEEKER_MODEL_HIDDEN1; ++j) {
        int64_t acc = static_cast<int64_t>(model.b1.getOrDefault(j, int32_t{0}));
        
        // Get row from w1
        const auto& row = model.w1.getOrDefault(j, Array<int16_t, SEEKER_MODEL_INPUTS>{});
        for (int i = 0; i < SEEKER_MODEL_INPUTS; ++i) {
            int16_t weight = row.getOrDefault(i, int16_t{0});
            int16_t inputValue = input.getOrDefault(i, int16_t{0});
            acc += static_cast<int64_t>(weight) * inputValue;
        }
        
        int32_t out = floorDiv(acc, 256); // Q8 scaling
        // Clamp manually since std::clamp doesn't work with mixed types
        if (out < -1048576) out = -1048576;
        if (out > 1048576) out = 1048576;
        hidden1.assignValue(j, out);
    }

    // Apply ReLU to hidden1 layer
    for (int j = 0; j < SEEKER_MODEL_HIDDEN1; ++j) {
        int32_t val = hidden1.getOrDefault(j, int32_t{0});
        if (val < 0) {
            hidden1.assignValue(j, int32_t{0});
        }
    }

    // Layer 2: hidden1 -> hidden2
    Array<int32_t, SEEKER_MODEL_HIDDEN2> hidden2;
    
    // Process w2 weights and biases
    for (int j = 0; j < SEEKER_MODEL_HIDDEN2; ++j) {
        int64_t acc = static_cast<int64_t>(model.b2.getOrDefault(j, int32_t{0}));
        
        // Get row from w2
        const auto& row = model.w2.getOrDefault(j, Array<int16_t, SEEKER_MODEL_HIDDEN1>{});
        for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
            int16_t weight = row.getOrDefault(i, int16_t{0});
            int32_t inputValue = hidden1.getOrDefault(i, int32_t{0});
            acc += static_cast<int64_t>(weight) * inputValue;
        }
        
        int32_t out = floorDiv(acc, 256); // Q8 scaling
        // Clamp manually since std::clamp doesn't work with mixed types
        if (out < -1048576) out = -1048576;
        if (out > 1048576) out = 1048576;
        hidden2.assignValue(j, out);
    }

    // Apply ReLU to hidden2 layer
    for (int j = 0; j < SEEKER_MODEL_HIDDEN2; ++j) {
        int32_t val = hidden2.getOrDefault(j, int32_t{0});
        if (val < 0) {
            hidden2.assignValue(j, int32_t{0});
        }
    }

    // Layer 3: hidden2 -> output
    // Process w3 weights and biases
    for (int j = 0; j < SEEKER_MODEL_OUTPUTS; ++j) {
        int64_t acc = static_cast<int64_t>(model.b3.getOrDefault(j, int32_t{0}));
        
        // Get row from w3
        const auto& row = model.w3.getOrDefault(j, Array<int16_t, SEEKER_MODEL_HIDDEN2>{});
        for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
            int16_t weight = row.getOrDefault(i, int16_t{0});
            int32_t inputValue = hidden2.getOrDefault(i, int32_t{0});
            acc += static_cast<int64_t>(weight) * inputValue;
        }
        
        int32_t out = floorDiv(acc, 256); // Q8 scaling
        // Clamp manually since std::clamp doesn't work with mixed types
        if (out < -1048576) out = -1048576;
        if (out > 1048576) out = 1048576;
        output.assignValue(j, out);
    }
}

SeekerModel makeRandomSeekerModel(uint64_t seed) {
    SeekerModel model;
    RngState rng{seed, 0};
    
    // Fill w1 row by row
    for (int j = 0; j < SEEKER_MODEL_HIDDEN1; ++j) {
        // Use access to get the row properly
        model.w1.access(j, [&](Array<int16_t, SEEKER_MODEL_INPUTS>& row) {
            for (int i = 0; i < SEEKER_MODEL_INPUTS; ++i) {
                uint32_t r = nextRandomValue(rng);
                int16_t weight = static_cast<int16_t>(static_cast<int>(r % 129) - 64);
                row.assignValue(i, weight);
            }
        });
    }

    // Fill b1
    for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
        model.b1.assignValue(i, int32_t{0});
    }

    // Fill w2 row by row
    for (int j = 0; j < SEEKER_MODEL_HIDDEN2; ++j) {
        // Use access to get the row properly
        model.w2.access(j, [&](Array<int16_t, SEEKER_MODEL_HIDDEN1>& row) {
            for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
                uint32_t r = nextRandomValue(rng);
                int16_t weight = static_cast<int16_t>(static_cast<int>(r % 129) - 64);
                row.assignValue(i, weight);
            }
        });
    }

    // Fill b2
    for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
        model.b2.assignValue(i, int32_t{0});
    }

    // Fill w3 row by row
    for (int j = 0; j < SEEKER_MODEL_OUTPUTS; ++j) {
        // Use access to get the row properly
        model.w3.access(j, [&](Array<int16_t, SEEKER_MODEL_HIDDEN2>& row) {
            for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
                uint32_t r = nextRandomValue(rng);
                int16_t weight = static_cast<int16_t>(static_cast<int>(r % 129) - 64);
                row.assignValue(i, weight);
            }
        });
    }

    // Fill b3
    for (int i = 0; i < SEEKER_MODEL_OUTPUTS; ++i) {
        model.b3.assignValue(i, int32_t{0});
    }

    return model;
}

std::string seekerModelBytes(const SeekerModel& model) {
    std::string bytes;
    bytes.reserve(SEEKER_MODEL_BYTES);
    
    // Layer 1 weights (w1): 32 rows × 43 columns = 1376 int16_t values
    for (int j = 0; j < SEEKER_MODEL_HIDDEN1; ++j) {
        const auto& row = model.w1.getOrDefault(j, Array<int16_t, SEEKER_MODEL_INPUTS>{});
        for (int i = 0; i < SEEKER_MODEL_INPUTS; ++i) {
            int16_t value = row.getOrDefault(i, int16_t{0});
            bytes.push_back(static_cast<char>(value & 0xFF));
            bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        }
    }

    // Layer 1 biases (b1): 32 int32_t values
    for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
        int32_t value = model.b1.getOrDefault(i, int32_t{0});
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 24) & 0xFF));
    }

    // Layer 2 weights (w2): 16 rows × 32 columns = 512 int16_t values
    for (int j = 0; j < SEEKER_MODEL_HIDDEN2; ++j) {
        const auto& row = model.w2.getOrDefault(j, Array<int16_t, SEEKER_MODEL_HIDDEN1>{});
        for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
            int16_t value = row.getOrDefault(i, int16_t{0});
            bytes.push_back(static_cast<char>(value & 0xFF));
            bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        }
    }

    // Layer 2 biases (b2): 16 int32_t values
    for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
        int32_t value = model.b2.getOrDefault(i, int32_t{0});
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 24) & 0xFF));
    }

    // Layer 3 weights (w3): 3 rows × 16 columns = 48 int16_t values
    for (int j = 0; j < SEEKER_MODEL_OUTPUTS; ++j) {
        const auto& row = model.w3.getOrDefault(j, Array<int16_t, SEEKER_MODEL_HIDDEN2>{});
        for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
            int16_t value = row.getOrDefault(i, int16_t{0});
            bytes.push_back(static_cast<char>(value & 0xFF));
            bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        }
    }

    // Layer 3 biases (b3): 3 int32_t values
    for (int i = 0; i < SEEKER_MODEL_OUTPUTS; ++i) {
        int32_t value = model.b3.getOrDefault(i, int32_t{0});
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<char>((value >> 24) & 0xFF));
    }

    return bytes;
}

bool seekerModelFromBytes(const std::string& bytes, SeekerModel& model) {
    if (bytes.size() != SEEKER_MODEL_BYTES) {
        return false;
    }

    size_t offset = 0;

    // Layer 1 weights (w1): 32 rows × 43 columns = 1376 int16_t values
    for (int j = 0; j < SEEKER_MODEL_HIDDEN1; ++j) {
        model.w1.access(j, [&](Array<int16_t, SEEKER_MODEL_INPUTS>& row) {
            for (int i = 0; i < SEEKER_MODEL_INPUTS; ++i) {
                int16_t value = static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset])) |
                                                     (static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8));
                row.assignValue(i, value);
                offset += 2;
            }
        });
    }

    // Layer 1 biases (b1): 32 int32_t values
    for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
        int32_t value = static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset])) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 2])) << 16) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 3])) << 24));
        model.b1.assignValue(i, value);
        offset += 4;
    }

    // Layer 2 weights (w2): 16 rows × 32 columns = 512 int16_t values
    for (int j = 0; j < SEEKER_MODEL_HIDDEN2; ++j) {
        model.w2.access(j, [&](Array<int16_t, SEEKER_MODEL_HIDDEN1>& row) {
            for (int i = 0; i < SEEKER_MODEL_HIDDEN1; ++i) {
                int16_t value = static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset])) |
                                                     (static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8));
                row.assignValue(i, value);
                offset += 2;
            }
        });
    }

    // Layer 2 biases (b2): 16 int32_t values
    for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
        int32_t value = static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset])) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 2])) << 16) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 3])) << 24));
        model.b2.assignValue(i, value);
        offset += 4;
    }

    // Layer 3 weights (w3): 3 rows × 16 columns = 48 int16_t values
    for (int j = 0; j < SEEKER_MODEL_OUTPUTS; ++j) {
        model.w3.access(j, [&](Array<int16_t, SEEKER_MODEL_HIDDEN2>& row) {
            for (int i = 0; i < SEEKER_MODEL_HIDDEN2; ++i) {
                int16_t value = static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset])) |
                                                     (static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8));
                row.assignValue(i, value);
                offset += 2;
            }
        });
    }

    // Layer 3 biases (b3): 3 int32_t values
    for (int i = 0; i < SEEKER_MODEL_OUTPUTS; ++i) {
        int32_t value = static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset])) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 2])) << 16) |
                                             (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 3])) << 24));
        model.b3.assignValue(i, value);
        offset += 4;
    }

    return true;
}

} // namespace hider_seeker