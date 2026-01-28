#pragma once

#include <functional>
#include "Array.hpp"
#include "CodeEnum.hpp"
#include "Keyframe.hpp"
#include "Location.hpp"
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
    static constexpr int MAX_KEYFRAMES = 6;
    int damage = 0;
    int role = 0;
    int feats = 0;
    int actions = 0;
    int moves = 0;

    // there are two problems to solve:
    // 1) should the backend store animation data?
    //  * if not, then a client opening a game will never know of the past 3 seconds of animations
    // 2) should characters store their animation data?
    //  * scales with characters, not animations
    //  * what's more likely? a) a game where all characters are animated, b) a game where only the moving characters are animated
    //  * (walking, hurtling), (attacking, casting), (hurting, dying), the likelihood of all of these at once is never.
    //      NPC characters might be doing 1-2 at any time. But animations could be queued up over longer times.
    //      NPC characters might all be performing an action at once, if the backend can determine scheduling between dependent actions.
    //          In the backend, all actions are worked through in time, but if off-screen characters are all performing jobs around the
    //          map, then rendering those animations might take up time that the players wait to resolve. One simple way to fix that
    //          would be to not have any NPC actions need to happen. So NPCs aren't designed to perform many things outside of view.
    //          But that 'outside of view' is misleading. NPCs are totally invisible to replay watchers and spectators, as well as 
    //          game designers. NPCs kind of adopt a totally different character, much like wild Pokemon in tall grass. If that can
    //          be abstracted to a form players like, then that's fine. That might be alright for a different game, but maybe not this
    //          one. 
    //      Maybe have a character buffer of [1-3] animations, if queued animations become a problem, then a global overflow.

    // one correct route, is for all animations to be keyframed.
    // so a character being hurt, dying, walking, jumping, these are all rendered in time.

    int visibility = ~0x0;
    Array<Keyframe, MAX_KEYFRAMES> keyframes;
    Location location;
    int characterId = -1;

    bool accessRole(CodeEnum &error, std::function<void(const RoleFlyweight &)>) const;
    bool isMovable(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isActor(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isActionable(CodeEnum &error, const bool isCheckingCount = false) const;
    bool isKeyer(CodeEnum &error) const;

    bool takeAction(CodeEnum &error); // Returns false if character is out of actions
    bool takeMove(CodeEnum &error);   // Returns false if character is out of moves
    bool takeFeat(CodeEnum &error);   // Returns false if character is out of feats

    void startTurn(Match &match); // Reset moves/actions to their initial values based on role
    void endTurn(Match &match);   // Clean up any end-of-turn effects
};