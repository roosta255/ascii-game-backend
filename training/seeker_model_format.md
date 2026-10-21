# Hider-Seeker Seeker Model File Format

This document describes the on-disk format for seeker models used in the hider-seeker game.

## Directory Layout

```
<root>/ACTIVE                     # one line: the active model_version
<root>/<model_version>/manifest.json
<root>/<model_version>/weights.bin
```

The production root is `var/models/seeker/` (5b3 wires it); every function takes the root as a parameter.

## File Format Details

### weights.bin

The `weights.bin` file contains the complete seeker model in binary format. It is exactly 4076 bytes long and uses little-endian byte order.

The layout consists of:
1. Layer 1 weights (w1): 32 rows × 43 columns = 1376 int16_t values
2. Layer 1 biases (b1): 32 int32_t values  
3. Layer 2 weights (w2): 16 rows × 32 columns = 512 int16_t values
4. Layer 2 biases (b2): 16 int32_t values
5. Layer 3 weights (w3): 3 rows × 16 columns = 48 int16_t values
6. Layer 3 biases (b3): 3 int32_t values

Each int16 value is stored as two bytes in little-endian order (LSB first).
Each int32 value is stored as four bytes in little-endian order (LSB first).

### manifest.json

The `manifest.json` file contains metadata about the model. It is a JSON object with the following fields:

- `"format"`: literal string value `"hider-seeker-seeker-mlp"`
- `"format_version"`: literal integer value `1`
- `"model_version"`: the version string passed to `writeSeekerModel`
- `"feature_version"`: literal integer value `1` (matching `SEEKER_FEATURE_VERSION`)
- `"layers"`: exact array `[43, 32, 16, 3]`
- `"fixed_point_shift"`: literal integer value `8`
- `"weights_fnv1a64"`: result of `fnv1a64Hex` applied to the bytes written to `weights.bin`
- `"created_by"`: literal string value `"seeker_model_init"`
- `"seed"`: the literal integer value passed as the seed argument to `writeSeekerModel`

## Inference Arithmetic

The model inference follows these steps:

1. **Accumulation**: For each layer, compute `acc = b[j] + Σ W[j][i]·x[i]` where:
   - `W[j][i]` is the weight from the weight matrix
   - `x[i]` is the input value (or output from previous layer)
   - `b[j]` is the bias for neuron j

2. **Fixed-point scaling**: Apply `floorDiv(acc, 256)` to convert from fixed-point Q8 representation to integer.

3. **Clamping**: Clamp results to the range [-1048576, 1048576].

4. **ReLU activation**: Apply ReLU (set negative values to zero) on hidden layers.

5. **Output interpretation**: The final outputs represent:
   - 0 = open
   - 1 = close  
   - 2 = wager
   All as Q8 scores read by `seeker_turn`.

## Feature Table

The input features are defined in `src/hider_seeker/seeker/feature_schema.hpp`. Input consists of:
- 39 server features (per active server)
- 4 global features (shared across all servers)

## Activation Process

To activate a model version, write the directory name to `<root>/ACTIVE` and then rewrite that file. The file contains exactly one line with the version string followed by a newline character.

## Example Usage

```cpp
// Writing a model
SeekerModel model = makeRandomSeekerModel(12345);
std::string error;
bool success = writeSeekerModel("var/models/seeker", "v1.0", model, 12345, true, error);

// Loading the active model  
SeekerModel loadedModel;
std::string version;
success = loadActiveSeekerModel("var/models/seeker", loadedModel, version, error);
```