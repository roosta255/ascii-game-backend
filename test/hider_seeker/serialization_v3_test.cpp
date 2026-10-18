#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <vector>
#include "state.hpp"
#include "serialization.hpp"
#include "match_end.hpp"

using namespace hider_seeker;

namespace {

std::vector<uint8_t> saved(const MatchState& state) {
    std::vector<uint8_t> buffer;
    saveMatchState(state, buffer);
    return buffer;
}

} // namespace

TEST_CASE("SerializationV3Test", "[hider_seeker][serialization]") {
    MatchState state;
    state.schema_version = 3;
    state.hider_connection_pool = 7;
    state.seeker_connection_mask = 0x8001;
    state.winner = static_cast<uint8_t>(MatchWinner::SEEKER);
    const std::vector<uint8_t> buffer = saved(state);

    SECTION("a v3 buffer round-trips the three new fields") {
        MatchState loaded;
        REQUIRE(loadMatchState(buffer, loaded));
        REQUIRE(loaded.hider_connection_pool == 7);
        REQUIRE(loaded.seeker_connection_mask == 0x8001);
        REQUIRE(loaded.winner == static_cast<uint8_t>(MatchWinner::SEEKER));
    }

    SECTION("a version-2 buffer is rejected") {
        // The buffer opens with the little-endian schema version marker.
        std::vector<uint8_t> v2 = buffer;
        v2[0] = 2;
        MatchState loaded;
        REQUIRE_FALSE(loadMatchState(v2, loaded));
    }

    SECTION("winner == 3 is rejected") {
        // Find the winner byte: the only byte that differs from a save
        // with winner NONE.
        MatchState none = state;
        none.winner = static_cast<uint8_t>(MatchWinner::NONE);
        const std::vector<uint8_t> other = saved(none);
        REQUIRE(other.size() == buffer.size());
        std::vector<size_t> differing;
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (buffer[i] != other[i]) {
                differing.push_back(i);
            }
        }
        REQUIRE(differing.size() == 1);

        std::vector<uint8_t> invalid = buffer;
        invalid[differing[0]] = 3;
        MatchState loaded;
        REQUIRE_FALSE(loadMatchState(invalid, loaded));
    }
}
