#include "SmokeSourceProvider.hpp"
#include "Match.hpp"
#include "TraitEnum.hpp"

namespace {
    constexpr int16_t SMOKE_PER_ENFLAMED_CHARACTER = 80;
}

int16_t SmokeSourceProvider::getRoomBaseValue(const Match& match, int roomId) const {
    int16_t value = 0;

    match.accessUsedCharacters([&](const Character& character) {
        if (character.location.roomId == roomId && character.traitsAfflicted[TRAIT_ENFLAMED].orElse(false)) {
            value += SMOKE_PER_ENFLAMED_CHARACTER;
        }
    });

    return value;
}
