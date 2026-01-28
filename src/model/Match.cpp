#include "Codeset.hpp"
#include "DoorFlyweight.hpp"
#include "GeneratorFlyweight.hpp"
#include "iActivator.hpp"
#include "iGenerator.hpp"
#include "iLayout.hpp"
#include "int2.hpp"
// #include "ItemFlyweight.hpp"
// #include "JsonParameters.hpp"
#include "Keyframe.hpp"
#include "make_filename.hpp"
#include "Match.hpp"
#include "RoleEnum.hpp"
// #include "RoleFlyweight.hpp"
// #include <json/json.h>
// #include <nlohmann/json.hpp>

constexpr size_t KILOBYTE = 1024;
static_assert(sizeof(Match) > KILOBYTE, "size milestone passed");
static_assert(sizeof(Match) < KILOBYTE * KILOBYTE, "size milestone passed");

// functions

void Match::accessAllCharacters(std::function<void(Character&)> consumer) {
    // NOTE: new character sets have to be added here
    for(auto& builder: builders) {
        consumer(builder.character);
    }
    for(auto& character: dungeon.characters) {
        consumer(character);
    }
}

void Match::accessAllCharacters(std::function<void(const Character&)> consumer) const {
    // NOTE: new character sets have to be added here
    for(const auto& builder: builders) {
        consumer(builder.character);
    }
    for(const auto& character: dungeon.characters) {
        consumer(character);
    }
}

void Match::accessUsedCharacters(std::function<void(const Character&)> consumer)const {
    accessAllCharacters([&](const Character& character){
        if (character.role != ROLE_EMPTY) {
            consumer(character);
        }
    });
}

bool Match::allocateCharacter(std::function<void(Character&)> consumer) {
    for (Character& character: dungeon.characters) {
        if (character.role == ROLE_EMPTY) {
            // Calculate the offset for the new character, guaranteed contained
            // this will be reassigned after the consumer
            int characterId;
            containsCharacter(character, characterId);
            consumer(character);
            character.characterId = characterId;
            return true;
        }
    }
    return false;
}

bool Match::findCharacter(int& characterId, std::function<bool(const Character&)> predicate)const {
    bool isFound = false;
    accessAllCharacters([&](const Character& character) {
        if (predicate(character)) {
            isFound = true;
            characterId = character.characterId;
        }
    });
    return isFound;
}

bool Match::accessPlayer
(const std::string& player, CodeEnum& error, std::function<void(Titan&)> titanConsumer, std::function<void(Builder&)> builderConsumer) {
    if (titan.player.account == player) {
        titanConsumer(titan);
        return true;
    }
    for (auto& builder: builders) {
        if (builder.player.account == player) {
            builderConsumer(builder);
            return true;
        }
    }
    error = CODE_INACCESSIBLE_PLAYER;
    return false;
}

bool Match::accessPlayers
( CodeEnum& error, std::function<void(Titan&, Player&)> consumeTitan, std::function<void(Builder&, Player&, const int)> consumeBuilder)
{
    if (!titan.player.isEmpty())
        consumeTitan(titan, titan.player);
    int i = 0;
    for (auto& builder: builders) {
        if (!builder.player.isEmpty())
            consumeBuilder(builder, builder.player, i++);
    }
    return true;
}

bool Match::setupSingleBuilder(CodeEnum& error) {
    // assuming TITAN_ONLY, addBuilder, removeTitan
    // migrate host to builder[0]
    turner.addBuilder();
    turner.removeTitan();
    accessPlayers(error,
        [](Titan&, Player& player){ player.account = ""; },
        [](Builder&, Player& player, const int){
            player.account = "";
        });
    builders.access(0, [&](Builder& builder){ builder.player.account = this->host; });
    return true;
}

bool Match::setupSingleTitan(CodeEnum& error) {
    // assuming TITAN_ONLY, noop
    // migrate host to titan
    return accessPlayers(error,
        [&](Titan&, Player& player){ player.account = this->host; },
        [](Builder&, Player& player, const int){ player.account = ""; });
}

bool Match::join(const std::string& joiner) {

    if (this->titan.player.account.empty()) {
        this->titan.player.account = joiner;
        turner.addTitan();
        return true;
    }

    for (auto& builder: builders) {
        if (builder.player.account.empty()) {
            builder.player.account = joiner;
            turner.addBuilder();
            return true;
        }
    }

    return false;
}

bool Match::start() {
    CodeEnum error;
    if (isStarted(error)) {
        return false;
    }

    return turner.startGame() == CODE_SUCCESS;
}

bool Match::containsCharacter(const Character& source, int& offset) const {
    // Get base address of this match object as a byte pointer
    auto base = reinterpret_cast<const char*>(this);

    // Get address of the source character
    auto ptr = reinterpret_cast<const char*>(&source);

    // Verify that the character lies within this Match object
    auto end = base + sizeof(Match);
    if (ptr >= base && ptr < end) {
        // Calculate offset
        offset = static_cast<int>(ptr - base);
        return true;
    }
    return false;
}

bool Match::containsOffset(int offset) const {
    return offset > 0 && offset < sizeof(Match);
}

Pointer<Character> Match::getCharacter (int offset, CodeEnum& error) {
    // Compute absolute address from offset
    auto base = reinterpret_cast<char*>(this);
    auto ptr = base + offset;

    if (containsOffset(offset))
        // Cast back to Character pointer
        return Pointer<Character>(*reinterpret_cast<Character*>(ptr));
    error = CODE_INACCESSIBLE_CHARACTER_ID;
    return Pointer<Character>::empty();
}

Pointer<Player> Match::getPlayer(const std::string& player, CodeEnum& error) {
    for (auto& builder: builders) {
        if (builder.player.account == player) {
            return builder.player;
        }
    }
    if (titan.player.account == player) {
        return titan.player;
    }
    error = CODE_INACCESSIBLE_PLAYER;
    return Pointer<Player>::empty();
}

bool Match::isStarted(CodeEnum& error) const {
    if (!turner.isStarted()) {
        error = CODE_UNSTARTED_MATCH;
        return false;
    }
    return true;
}

bool Match::isCompleted(CodeEnum& error) const {
    if (!turner.isCompleted()) {
        error = CODE_UNCOMPLETED_MATCH;
        return false;
    }
    return true;
}

bool Match::isFull(CodeEnum& error) const {
    if (this->titan.player.account.empty()) {
        return false;
    }
    for (const auto& builder: this->builders) {
        if (builder.player.account.empty()) {
            return false;
        }
    }
    return true;
}

bool Match::isEmpty(CodeEnum& error) const {
    if (!this->titan.player.account.empty()) {
        return false;
    }
    for (const auto& builder: this->builders) {
        if (!builder.player.account.empty()) {
            return false;
        }
    }
    return true;
}

bool Match::leave(const std::string& playerId, CodeEnum& error) {
    return accessPlayer(playerId, error, [&](Titan& titan) {
        titan.player.account = "";
        turner.removeTitan();
    }, [&](Builder& builder) {
        builder.player.account = "";
        turner.removeBuilder(playerId, *this);
    });
}

bool Match::endTurn(const std::string& playerId, CodeEnum& error)
{
    return accessPlayer(playerId, error, [&](Titan& titan) {
        turner.endTitanTurn(*this);
    }, [&](Builder& builder) {
        turner.endBuilderTurn(*this);
    });
}

void Match::setFilename() {
    filename = make_filename("match-" + host + "-" + username);
}
