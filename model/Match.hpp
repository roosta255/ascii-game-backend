#pragma once

#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "Dungeon.hpp"
#include "MovementEnum.hpp"
#include "Pointer.hpp"
#include "Titan.hpp"
#include <string>

constexpr int MATCH_BUILDER_COUNT = 3;

namespace Json {
    class Value;
}

struct Match {
    std::string match;
    std::string host;
    uint32_t version = 0;
    uint32_t turn = 0;
    bool started = false;
    int generator = 0;

    Titan titan;
    Array<Builder, MATCH_BUILDER_COUNT> builders;
    Dungeon dungeon;

    bool create(const std::string& titan);
    bool join(const std::string& builder);
    bool start();
    bool generate(int seed, Match& output);
    MovementEnum moveCharacterToWall(int roomId, int characterId, Cardinal);

    bool containsCharacter(const Character&, int& offset) const;
    bool containsOffset(int offset) const;
    Pointer<Character> getCharacter(int offset);

    void toJson(Json::Value& out) const;
    bool fromJson(const Json::Value& in);
};
