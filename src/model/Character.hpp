#pragma once

#include <functional>
#include "CodeEnum.hpp"
#include "Pointer.hpp"

namespace Json
{
    class Value;
}

class JsonParameters;
class Match;
class RoleFlyweight;
class Room;

struct Character
{
    int damage = 0;
    int role = 0;
    int feats = 0;
    int actions = 0;
    int moves = 0;
    int keys = 0; // Number of keys held by the character
    Pointer<Room> room;
    int x;
    int y;
    int visibility = ~0x0;

    bool accessRole(CodeEnum &error, std::function<void(const RoleFlyweight &)>) const;
    bool isMovable(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isActor(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isActionable(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isKeyer(CodeEnum &error, const bool isCheckingCount = false) const;

    bool takeAction(CodeEnum &error); // Returns false if character is out of actions
    bool takeMove(CodeEnum &error);   // Returns false if character is out of moves
    bool takeFeat(CodeEnum &error);   // Returns false if character is out of feats
    bool takeKey(CodeEnum &error);    // Returns false if character has no keys
    bool giveKey(CodeEnum &error);    // Returns false if character can't receive keys

    void startTurn(Match &match); // Reset moves/actions to their initial values based on role
    void endTurn(Match &match);   // Clean up any end-of-turn effects
};