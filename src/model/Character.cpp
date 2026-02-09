#include "Character.hpp"
#include "CharacterDigest.hpp"
#include "JsonParameters.hpp"
#include "Match.hpp"
#include <json/json.h>
#include "RoleFlyweight.hpp"

bool Character::getDigest(CodeEnum& error, CharacterDigest& digest)const {
    return accessRole(error, [&](const RoleFlyweight& flyweight){
        digest = CharacterDigest{
            .healthRemaining = Maybe<int>(flyweight.health - this->damage),
            .featsRemaining = Maybe<int>(flyweight.feats - this->feats),
            .actionsRemaining = flyweight.isActor || flyweight.isActionable ? Maybe<int>(flyweight.actions - this->actions) : Maybe<int>::empty(),
            .movesRemaining = flyweight.isMovable ? Maybe<int>(flyweight.moves - this->moves) : Maybe<int>::empty()
        };
    });
}

bool Character::isMovable(CodeEnum& error, const bool isCheckingCount) const
{
    bool result = false;
    int movesTaken = moves;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (!flyweight.isMovable)
            error = CODE_CHARACTER_NOT_MOVABLE;
        else if (isCheckingCount && flyweight.moves - movesTaken <= 0) 
            error = CODE_CHARACTER_OUT_OF_MOVES;
        else result = true;
    });
    return result;
}

bool Character::isActor(CodeEnum& error, const bool isCheckingCount) const
{
    bool result = false;
    int actionsTaken = actions;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (!flyweight.isActor) error = CODE_CHARACTER_NOT_ACTOR;
        else if (isCheckingCount && flyweight.actions - actionsTaken <= 0)
            error = CODE_CHARACTER_OUT_OF_ACTIONS;
        else result = true;
    });
    return result;
}

bool Character::isActionable(CodeEnum& error, const bool isCheckingCount) const
{
    bool result = false;
    int actionsTaken = actions;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (!flyweight.isActionable) error = CODE_CHARACTER_NOT_ACTIONABLE;
        else if (isCheckingCount && flyweight.actions - actionsTaken <= 0)
            error = CODE_CHARACTER_OUT_OF_ACTIONS;
        else result = true;
    });
    return result;
}

bool Character::isKeyer(CodeEnum& error) const
{
    bool result = false;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (!flyweight.isKeyer)
            error = CODE_CHARACTER_NOT_KEYER;
        else result = true;
    });
    return result;
}

bool Character::takeAction(CodeEnum& error)
{
    bool result = false;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (actions < flyweight.actions) {
            result = true;
            actions++;
        } else {
            error = CODE_CHARACTER_OUT_OF_ACTIONS;
        }
    });
    return result;
}

bool Character::takeMove(CodeEnum& error)
{
    bool result = false;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (moves < flyweight.moves) {
            result = true;
            moves++;
        } else {
            error = CODE_CHARACTER_OUT_OF_MOVES;
        }
    });
    return result;
}

bool Character::takeFeat(CodeEnum& error)
{
    bool result = false;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (feats < flyweight.feats) {
            result = true;
            feats++;
        } else {
            error = CODE_CHARACTER_OUT_OF_FEATS;
        }
    });
    return result;
}

bool Character::accessRole
(
    CodeEnum& error,
    std::function<void(const RoleFlyweight&)> consumer
) const {
    if (RoleFlyweight::getFlyweights().accessConst(role, [&](const RoleFlyweight& flyweight){
        consumer(flyweight);
    })) {
        return true;
    }
    error = CODE_INACCESSIBLE_CHARACTER_ROLE;
    return false;
}

void Character::startTurn(Match& match)
{
    moves = 0;
    actions = 0;
}

void Character::endTurn(Match& match)
{
    // Currently no end-of-turn cleanup needed
}

std::ostream& operator<<(std::ostream& os, const Character& rhs) {
    std::hash<Character> temp;
    
    return os << ", character:" << temp(rhs) << "{ "
            << " location: " << rhs.location
            << ", damage:" << rhs.damage
            << ", role:" << rhs.role
            << ", feats:" << rhs.feats
            << ", actions:" << rhs.actions
            << ", moves:" << rhs.moves
            << " }";
}
