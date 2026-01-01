#include "Character.hpp"
#include "JsonParameters.hpp"
#include "Match.hpp"
#include <json/json.h>
#include "RoleFlyweight.hpp"

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

bool Character::isKeyer(CodeEnum& error, const bool isCheckingCount) const
{
    bool result = false;
    accessRole(error, [&](const RoleFlyweight& flyweight){
        if (!flyweight.isKeyer)
            error = CODE_CHARACTER_NOT_KEYER;
        else if (isCheckingCount && keys <= 0)
            error = CODE_CHARACTER_OUT_OF_KEYS;
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

bool Character::takeKey(CodeEnum& error)
{
    if (keys > 0) {
        keys--;
        return true;
    }
    error = CODE_CHARACTER_OUT_OF_KEYS;
    return false;
}

bool Character::giveKey(CodeEnum& error)
{
    if (!isKeyer(error)) {
        return false;
    }
    keys++;
    return true;
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
