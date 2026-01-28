#pragma once

#include "Array.hpp"
#include "Builder.hpp"
#include "Cardinal.hpp"
#include "CodeEnum.hpp"
#include "Dungeon.hpp"
#include "Pointer.hpp"
#include "Titan.hpp"
#include "Turner.hpp"
#include <string>

class Codeset;

struct Match {
    constexpr static int MATCH_BUILDER_COUNT = 3;

    std::string filename = "ERROR_UNSET_FILENAME";
    std::string username;
    std::string host;
    uint32_t version = 0;
    int generator = 0;

    Titan titan;
    Array<Builder, MATCH_BUILDER_COUNT> builders;
    Dungeon dungeon;
    Turner turner;

    // functions
    void accessAllCharacters(std::function<void(Character&)> consumer);
    void accessAllCharacters(std::function<void(const Character&)> consumer)const;
    void accessUsedCharacters(std::function<void(const Character&)> consumer)const;
    bool allocateCharacter(std::function<void(Character&)> consumer);

    bool findCharacter(int& characterId, std::function<bool(const Character&)> predicate)const;

    bool join(const std::string& joiner);
    bool leave(const std::string& leaver, CodeEnum& error);
    bool start();
    bool setupSingleBuilder(CodeEnum& output);
    bool setupSingleTitan(CodeEnum& output);

    bool endTurn(const std::string& playerId, CodeEnum& error);

    bool isStarted(CodeEnum& result) const;
    bool isCompleted(CodeEnum& result) const;
    bool isFull(CodeEnum& result) const;
    bool isEmpty(CodeEnum& result) const;

    bool containsCharacter(const Character&, int& offset) const;
    bool containsOffset(int offset) const;
    Pointer<Character> getCharacter(int offset, CodeEnum&);
    Pointer<Player> getPlayer(const std::string& player, CodeEnum&);
    bool accessPlayer(const std::string& player, CodeEnum&, std::function<void(Titan&)>, std::function<void(Builder&)>);

    bool accessPlayers(CodeEnum&, std::function<void(Titan&, Player&)>, std::function<void(Builder&, Player&, const int)>);

    void setFilename();

};
